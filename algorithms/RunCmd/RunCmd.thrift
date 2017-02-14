/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators ../alg_common/AlgCommon.thrift
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators RunCmd.thrift
 */

include "../alg_common/AlgCommon.thrift"

namespace * RunCmd

service RunCmdService {
    string runCmd( 1:string cmd ) throws (1:AlgCommon.InvalidRequest err)
}
