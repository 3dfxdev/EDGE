/* Rise of the Triad MAPS */

/// FIXME: Refactor using EPI file handling, C++-isms, etc.

#include "i_defs.h"

#include <stdio.h>

#include <vector>

#include "../epi/endianess.h"

#include "rott_local.h" ///FIXME: This should just be rott_local.h ... 

/*---------------------------------------------------------------------
   Map constants
---------------------------------------------------------------------*/

#define MAXLEVELNAMELENGTH           23
#define ALLOCATEDLEVELNAMELENGTH     24
#define NUMPLANES                    3
#define NUMHEADEROFFSETS             100
#define MAPWIDTH                     128
#define MAPHEIGHT                    128
#define MAP_SPECIAL_TOGGLE_PUSHWALLS 0x0001

#define WALL_PLANE    0
#define SPRITE_PLANE  1
#define INFO_PLANE    2

/*---------------------------------------------------------------------
   Type definitions
---------------------------------------------------------------------*/

typedef struct
{
   unsigned long used;
   unsigned long CRC;
   unsigned long RLEWtag;
   unsigned long MapSpecials;
   unsigned long planestart[ NUMPLANES ];
   unsigned long planelength[ NUMPLANES ];
   char          Name[ ALLOCATEDLEVELNAMELENGTH ];
} RTLMAP;


/*---------------------------------------------------------------------
   Global variables
---------------------------------------------------------------------*/

unsigned short *mapplanes[ NUMPLANES ];


/*---------------------------------------------------------------------
   Macros
---------------------------------------------------------------------*/

#define MAPSPOT( x, y, plane ) \
   ( mapplanes[ plane ][ MAPWIDTH * ( y ) + ( x ) ] )

#define WALL_AT( x, y )   ( MAPSPOT( ( x ), ( y ), WALL_PLANE ) )
#define SPRITE_AT( x, y ) ( MAPSPOT( ( x ), ( y ), SPRITE_PLANE ) )
#define INFO_AT( x, y )   ( MAPSPOT( ( x ), ( y ), INFO_PLANE ) )


/*---------------------------------------------------------------------
   Function: ReadROTTMap

   Read a map from a RTL/RTC file.
   CA~ 6.3.2016 - Use epi for file reading (!!!)
   Ew, ugly hacks inbound!!!!!!
---------------------------------------------------------------------*/

void ReadROTTMap(void)
{
   char *filename,
   int mapnum
   char            RTLSignature[ 4 ];
   unsigned long   RTLVersion;
   RTLMAP          RTLMap;
   int             filehandle;
   long            pos;
   long            compressed;
   long            expanded;
   int             plane;
   unsigned short *buffer;

   filehandle = open( filename, O_RDONLY | O_BINARY );

   //
   // Load RTL signature
   //
   read( filehandle, RTLSignature, sizeof( RTLSignature ) );

   //
   // Read the version number
   //
   read( filehandle, &RTLVersion, sizeof( RTLVersion ) );

   //
   // Load map header
   //
   lseek( filehandle, mapnum * sizeof( RTLMap ), SEEK_CUR );
   read( filehandle, &RTLMap, sizeof( RTLMap ) );

   if ( !RTLMap.used )
      {
      //
      // Exit on error
      //
      I_Error( "ReadROTTMap: Tried to load a non existent map!" );
      }

   //
   // load the planes in (!!! MAPWIDTH/MAPHEIGHT are ROTT_planes, unlike Wolf3D)
   //
   expanded = MAPWIDTH * MAPHEIGHT * 2;

	for( plane = 0; plane <= 2; plane++ )
      {
      pos        = RTLMap.planestart[ plane ];
      compressed = RTLMap.planelength[ plane ];
      buffer     = malloc( compressed );

      lseek( filehandle, pos, SEEK_SET );
      read( filehandle, buffer, compressed );

      mapplanes[ plane ] = malloc( expanded );

      RLEW_Expand( buffer, mapplanes[ plane ], expanded >> 1, RTLMap.RLEWtag ); ///yuck.

      free( buffer );
      }

   close( filehandle );

}

