//----------------------------------------------------------------------------
//  EDGE Linux Compensate Functions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
// This file accounts for functions in
// that are part of some standard libraries
// but not others.
//
// -ACB- 1999/10/10
//

#ifndef O_BINARY
#define O_BINARY  0
#endif

#ifndef D_OK
#define D_OK  X_OK
#endif

#define stricmp   strcasecmp
#define strnicmp  strncasecmp

void strupr(char *str);
int strncasecmpwild(const char *s1, const char *s2, int n);

#ifdef ASM_NEED_UNDERSCORES
#define columnofs       _columnofs
#define loopcount       _loopcount
#define pixelcount      _pixelcount
#define dc_colourmap    _dc_colourmap
#define dc_source       _dc_source
#define dc_x            _dc_x
#define dc_yl           _dc_yl
#define dc_yh           _dc_yh
#define dc_yfrac        _dc_yfrac
#define dc_ystep        _dc_ystep
#define ds_colourmap    _ds_colourmap
#define ds_source       _ds_source
#define ds_x1           _ds_x1
#define ds_x2           _ds_x2
#define ds_xfrac        _ds_xfrac
#define ds_xstep        _ds_xstep
#define ds_y            _ds_y
#define ds_yfrac        _ds_yfrac
#define ds_ystep        _ds_ystep
#define dv_viewwidth    _dv_viewwidth
#define dv_viewheight   _dv_viewheight
#define dv_ylookup      _dv_ylookup
#define dv_columnofs    _dv_columnofs
#define vb_depth        _vb_depth
#define ylookup         _ylookup

#define asmret                    _asmret
#define asmarg1                   _asmarg1
#define asmarg2                   _asmarg2
#define I_386GetCpuidInfo         _I_386GetCpuidInfo
#define I_386Is486                _I_386Is486
#define I_386IsCpuidSupported     _I_386IsCpuidSupported
#define I_386IsCyrix              _I_386IsCyrix
#define I_386IsFPU                _I_386IsFPU

#define R_DrawColumn16_chi        _R_DrawColumn16_chi
#define R_DrawColumn16_rasem      _R_DrawColumn16_rasem
#define R_DrawColumn8_chi         _R_DrawColumn8_chi
#define R_DrawColumn8_id          _R_DrawColumn8_id
#define R_DrawColumn8_id_end      _R_DrawColumn8_id_end
#define R_DrawColumn8_id_erik     _R_DrawColumn8_id_erik
#define R_DrawColumn8_id_erik_end _R_DrawColumn8_id_erik_end
#define R_DrawColumn8_mmx_k6      _R_DrawColumn8_mmx_k6
#define R_DrawColumn8_mmx_k6_end  _R_DrawColumn8_mmx_k6_end
#define R_DrawColumn8_pentium     _R_DrawColumn8_pentium
#define R_DrawColumn8_rasem       _R_DrawColumn8_rasem
#define R_DrawSpan16_rasem        _R_DrawSpan16_rasem
#define R_DrawSpan8_id            _R_DrawSpan8_id
#define R_DrawSpan8_id_end        _R_DrawSpan8_id_end
#define R_DrawSpan8_id_erik       _R_DrawSpan8_id_erik
#define R_DrawSpan8_id_erik_end   _R_DrawSpan8_id_erik_end
#define R_DrawSpan8_mmx           _R_DrawSpan8_mmx
#define R_DrawSpan8_mmx_end       _R_DrawSpan8_mmx_end
#define R_DrawSpan8_rasem         _R_DrawSpan8_rasem
#define R_EnlargeView8_2_2_base   _R_EnlargeView8_2_2_base
#define R_EnlargeView8_2_2_mmx    _R_EnlargeView8_2_2_mmx

#define rdc16owidth1       _rdc16owidth1
#define rdc16owidth2       _rdc16owidth2
#define rdc8echecklast     _rdc8echecklast
#define rdc8edone          _rdc8edone
#define rdc8eloop          _rdc8eloop
#define rdc8eoffs1         _rdc8eoffs1
#define rdc8eoffs2         _rdc8eoffs2
#define rdc8eoffs3         _rdc8eoffs3
#define rdc8epatch1        _rdc8epatch1
#define rdc8epatch2        _rdc8epatch2
#define rdc8epatch3        _rdc8epatch3
#define rdc8epatch4        _rdc8epatch4
#define rdc8epatcher1      _rdc8epatcher1
#define rdc8epatcher2      _rdc8epatcher2
#define rdc8epatcher3      _rdc8epatcher3
#define rdc8epatcher4      _rdc8epatcher4
#define rdc8ewidth1        _rdc8ewidth1
#define rdc8ewidth2        _rdc8ewidth2
#define rdc8ichecklast     _rdc8ichecklast
#define rdc8idone          _rdc8idone
#define rdc8iloop          _rdc8iloop
#define rdc8ioffs1         _rdc8ioffs1
#define rdc8ioffs2         _rdc8ioffs2
#define rdc8ioffs3         _rdc8ioffs3
#define rdc8ipatch1        _rdc8ipatch1
#define rdc8ipatch2        _rdc8ipatch2
#define rdc8ipatcher1      _rdc8ipatcher1
#define rdc8ipatcher2      _rdc8ipatcher2
#define rdc8iwidth1        _rdc8iwidth1
#define rdc8iwidth2        _rdc8iwidth2
#define rdc8mdone          _rdc8mdone
#define rdc8mloop          _rdc8mloop
#define rdc8moffs1         _rdc8moffs1
#define rdc8moffs2         _rdc8moffs2
#define rdc8mwidth1        _rdc8mwidth1
#define rdc8mwidth2        _rdc8mwidth2
#define rdc8mwidth3        _rdc8mwidth3
#define rdc8mwidth4        _rdc8mwidth4
#define rdc8mwidth5        _rdc8mwidth5
#define rdc8nwidth1        _rdc8nwidth1
#define rdc8nwidth2        _rdc8nwidth2
#define rdc8pwidth1        _rdc8pwidth1
#define rdc8pwidth2        _rdc8pwidth2
#define rdc8pwidth3        _rdc8pwidth3
#define rdc8pwidth4        _rdc8pwidth4
#define rdc8pwidth5        _rdc8pwidth5
#define rdc8pwidth6        _rdc8pwidth6
#define rds8echecklast     _rds8echecklast
#define rds8edone          _rds8edone
#define rds8eloop          _rds8eloop
#define rds8eoffs1         _rds8eoffs1
#define rds8eoffs2         _rds8eoffs2
#define rds8eoffs3         _rds8eoffs3
#define rds8epatch1        _rds8epatch1
#define rds8epatch2        _rds8epatch2
#define rds8epatch3        _rds8epatch3
#define rds8epatch4        _rds8epatch4
#define rds8epatcher1      _rds8epatcher1
#define rds8epatcher2      _rds8epatcher2
#define rds8epatcher3      _rds8epatcher3
#define rds8epatcher4      _rds8epatcher4
#define rds8ichecklast     _rds8ichecklast
#define rds8idone          _rds8idone
#define rds8iloop          _rds8iloop
#define rds8ioffs1         _rds8ioffs1
#define rds8ioffs2         _rds8ioffs2
#define rds8ioffs3         _rds8ioffs3
#define rds8ipatch1        _rds8ipatch1
#define rds8ipatch2        _rds8ipatch2
#define rds8ipatcher1      _rds8ipatcher1
#define rds8ipatcher2      _rds8ipatcher2
#define rds8mdone          _rds8mdone
#define rds8mloop          _rds8mloop
#define rds8moffs1         _rds8moffs1
#endif

