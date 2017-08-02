cmake_minimum_required( VERSION 2.8.7 )

add_subdirectory( updaterevision )
add_subdirectory( eecrashreport )


set( CROSS_EXPORTS ${CROSS_EXPORTS} PARENT_SCOPE )
