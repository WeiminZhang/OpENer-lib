opENer_common_includes()


set( CIP_TEST_SRC TEST_Cip_Connection.hpp TEST_Cip_Connection.cpp)

add_executable( TEST_CIP_CLASS0005_CONNECTION ${CIP_TEST_SRC})
target_link_libraries (TEST_CIP_CLASS0005_CONNECTION OpENerLib)

add_test(UNITTEST_CIP_CLASS0005_CONNECTION TEST_CIP_CLASS0005_CONNECTION )