/*
 * acq400_HT_server.cpp
 *
 *  Created on: 18 Mar 2026
 *      Author: pgm
 */

#ifdef PVXS_AI_EXAMPLE_COMPILES

#include <pvxs/server.h>
#include <pvxs/data.h>
using namespace pvxs;

static auto GroupArrayType = pvxs::TypeDef(pvxs::TypeCode::Struct, std::string("grp:array:1.0"));
/*
    .add("value", TypeCode::StructA, "grp:elem:1.0")  // structure[]
        .add("name",   TypeCode::String)
        .add("x",      TypeCode::Float64)
        .add("y",      TypeCode::Float64)
        .add("status", TypeCode::Int32)
    .end()
    .create();
*/

Value makeGroupArray()
{
    Value top = GroupArrayType.create();         // structure with field "value"
    Value arr = top["value"];                    // structure[]

    // Build array of two group elements
    shared_array<Value> elems(2);

    elems[0] = GroupArrayType["value"].create();
    elems[0]["name"]   = "grp0";
    elems[0]["x"]      = 1.23;
    elems[0]["y"]      = 4.56;
    elems[0]["status"] = 0;

    elems[1] = GroupArrayType["value"].create();
    elems[1]["name"]   = "grp1";
    elems[1]["x"]      = 7.89;
    elems[1]["y"]      = 0.12;
    elems[1]["status"] = 1;

    arr = elems.freeze();                        // assign structure[]

    return top;
}



#include <pvxs/server.h>
#include <pvxs/sharedpv.h>
using namespace pvxs;

#endif

int main()
{
#ifdef PVXS_AI_EXAMPLE_COMPILES
    auto src = server::SharedPV::buildReadonly()
        .open(makeGroupArray())
        .exec();

    auto serv = server::Config::fromEnv()
        .build()
        .addPV("MY:GROUPS", src);

    serv.run();      // or loop on serv.process()
#endif
    return 0;
}


