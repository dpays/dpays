#include <steemit/market_history/market_history_api.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/history_object.hpp>

namespace steemit { namespace market_history {

namespace detail {

class market_history_plugin_impl
{
   public:
      market_history_plugin_impl( market_history_plugin& plugin )
         :_self( plugin ) {}
      virtual ~market_history_plugin_impl() {}

      /**
       * This method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_market_histories( const operation_object& b );

      market_history_plugin& _self;
      flat_set<uint32_t>     _tracked_buckets = { 15, 60, 300, 3600, 86400 };
      int32_t                _maximum_history_per_bucket_size = 1000;
};

void market_history_plugin_impl::update_market_histories( const operation_object& o )
{
   if( o.op.which() == operation::tag< fill_order_operation >::value )
   {
      fill_order_operation op = o.op.get< fill_order_operation >();

      auto& db = _self.database();
      const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_bucket >();

      uint64_t history_seq = std::numeric_limits< uint64_t >::min();

      db.create< order_history_object >( [&]( order_history_object& ho )
      {
         ho.time = db.head_block_time();
         ho.op = op;
      });

      if( !_maximum_history_per_bucket_size ) return;
      if( !_tracked_buckets.size() ) return;
      if( op.pays.symbol != STEEM_SYMBOL ) return;

      for( auto bucket : _tracked_buckets )
      {
         auto cutoff = db.head_block_time() - fc::seconds( bucket * _maximum_history_per_bucket_size );

         auto open = fc::time_point_sec( ( db.head_block_time().sec_since_epoch() / bucket ) * bucket );
         auto seconds = bucket;

         auto itr = bucket_idx.find( boost::make_tuple( seconds, open ) );
         if( itr == bucket_idx.end() )
         {
            db.create< bucket_object >( [&]( bucket_object &b )
            {
               b.open = open;
               b.seconds = bucket;
               b.high_steem = op.pays.amount;
               b.high_sbd = op.receives.amount;
               b.low_steem = op.pays.amount;
               b.low_sbd = op.receives.amount;
               b.open_steem = op.pays.amount;
               b.open_sbd = op.receives.amount;
               b.close_steem = op.pays.amount;
               b.close_sbd = op.receives.amount;
               b.steem_volume = op.pays.amount;
               b.sbd_volume = op.receives.amount;
            });
         }
         else
         {
            db.modify( *itr, [&]( bucket_object& b ){
               b.steem_volume += op.pays.amount;
               b.sbd_volume += op.receives.amount;
               b.close_steem = op.pays.amount;
               b.close_sbd = op.receives.amount;

               if( b.high() < price( op.receives, op.pays ) )
               {
                  b.high_steem = op.pays.amount;
                  b.high_sbd = op.receives.amount;
               }

               if( b.low() > price( op.receives, op.pays ) )
               {
                  b.low_steem = op.pays.amount;
                  b.low_sbd = op.receives.amount;
               }
            });

            if( _maximum_history_per_bucket_size > 0 )
            {
               open = fc::time_point_sec();
               itr = bucket_idx.lower_bound( boost::make_tuple( seconds, open ) );

               while( itr->seconds == seconds && itr->open < cutoff )
               {
                  auto old_itr = itr;
                  ++itr;
                  db.remove( *old_itr );
               }
            }
         }
      }
   }
}

} // detail

market_history_plugin::market_history_plugin() :
   _my( new detail::market_history_plugin_impl( *this ) ) {}
market_history_plugin::~market_history_plugin() {}

void market_history_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cli.add_options()
         ("bucket-size", boost::program_options::value<string>()->default_value("[15,60,300,3600,86400]"),
           "Track market history by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
         ("history-per-size", boost::program_options::value<uint32_t>()->default_value(1000),
           "How far back in time to track history for each bucket size, measured in the number of buckets (default: 1000)")
         ;
   cfg.add(cli);
}

void market_history_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      database().on_applied_operation.connect( [&]( const operation_object& o ){ _my->update_market_histories( o ); } );
      database().add_index< primary_index< bucket_index > >();
      database().add_index< primary_index< order_history_index > >();

      if( options.count("bucket-size" ) )
      {
         const std::string& buckets = options["bucket-size"].as< string >();
         _my->_tracked_buckets = fc::json::from_string( buckets ).as< flat_set< uint32_t > >();
      }
      if( options.count("history-per-size" ) )
         _my->_maximum_history_per_bucket_size = options["history-per-size"].as< uint32_t >();
   } FC_CAPTURE_AND_RETHROW()
}

void market_history_plugin::plugin_startup()
{
   app().register_api_factory< market_history_api >( "market_history_api" );
}

flat_set< uint32_t > market_history_plugin::get_tracked_buckets() const
{
   return _my->_tracked_buckets;
}

uint32_t market_history_plugin::get_max_history_per_bucket() const
{
   return _my->_maximum_history_per_bucket_size;
}

} } // steemit::market_history

STEEMIT_DEFINE_PLUGIN( market_history, steemit::market_history::market_history_plugin )