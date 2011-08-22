/* vi:set ts=8 sts=4 sw=4 tw=0: */
/*
 * cdecl.h - Cä÷êîêÈåæÉ}ÉNÉç
 *
 * Last Change: 06-Oct-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */
#ifndef INCLUDED_CDECL_H
#define INCLUDED_CDECL_H

#ifdef __cplusplus
# define C_DECL_BEGIN() extern "C" {
# define C_DECL_END() }
#else
# define C_DECL_BEGIN()
# define C_DECL_END()
#endif

C_DECL_BEGIN();
C_DECL_END();

#endif /* INCLUDED_CDECL_H */
