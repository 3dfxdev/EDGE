cmake_minimum_required( VERSION 2.8.7 )

add_subdirectory( updaterevision )

#if(MSVC)
#add_subdirectory( eecrashreport )
#endif()

set( CROSS_EXPORTS ${CROSS_EXPORTS} PARENT_SCOPE )
