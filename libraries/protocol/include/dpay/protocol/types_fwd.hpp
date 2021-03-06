#pragma once

namespace fc {
class uint128;
class variant;
} // fc

namespace dpay { namespace protocol {
template< typename Storage = fc::uint128 >
class fixed_string_impl;

class asset_symbol_type;
class legacy_dpay_asset_symbol_type;
struct legacy_dpay_asset;
} } // dpay::protocol

namespace fc { namespace raw {

template<typename Stream>
inline void pack( Stream& s, const uint128& u );
template<typename Stream>
inline void unpack( Stream& s, uint128& u );

template< typename Stream, typename Storage >
inline void pack( Stream& s, const dpay::protocol::fixed_string_impl< Storage >& u );
template< typename Stream, typename Storage >
inline void unpack( Stream& s, dpay::protocol::fixed_string_impl< Storage >& u );

template< typename Stream >
inline void pack( Stream& s, const dpay::protocol::asset_symbol_type& sym );
template< typename Stream >
inline void unpack( Stream& s, dpay::protocol::asset_symbol_type& sym );

template< typename Stream >
inline void pack( Stream& s, const dpay::protocol::legacy_dpay_asset_symbol_type& sym );
template< typename Stream >
inline void unpack( Stream& s, dpay::protocol::legacy_dpay_asset_symbol_type& sym );

} // raw

template< typename Storage >
inline void to_variant( const dpay::protocol::fixed_string_impl< Storage >& s, fc::variant& v );
template< typename Storage >
inline void from_variant( const variant& v, dpay::protocol::fixed_string_impl< Storage >& s );

inline void to_variant( const dpay::protocol::asset_symbol_type& sym, fc::variant& v );

inline void from_variant( const fc::variant& v, dpay::protocol::legacy_dpay_asset& leg );
inline void to_variant( const dpay::protocol::legacy_dpay_asset& leg, fc::variant& v );

} // fc
