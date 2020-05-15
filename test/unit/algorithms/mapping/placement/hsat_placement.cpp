/*------------------------------------------------------------------------------
| Part of the tweedledum.  This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*-----------------------------------------------------------------------------*/
#include "tweedledum/algorithms/mapping/placement/hsat_placement.h"

#include "../test_circuits.h"
#include "tweedledum/algorithms/verification/placement_verify.h"
#include "tweedledum/ir/CircuitDAG.h"
#include "tweedledum/ir/Gate.h"
#include "tweedledum/ir/MappedDAG.h"
#include "tweedledum/ir/Netlist.h"
#include "tweedledum/ir/Operation.h"
#include "tweedledum/ir/Wire.h"
#include "tweedledum/target/Device.h"

#include <catch.hpp>

using namespace tweedledum;

TEMPLATE_TEST_CASE("Test for SAT intial placement heuristic",
    "[hsat_placement][mapping]", Netlist, CircuitDAG)
{
	TestType network;
	SECTION("Empty network")
	{
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
	SECTION("Test circuit 00")
	{
		network = test_circuit_00<TestType>();
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
	SECTION("Test circuit 01")
	{
		network = test_circuit_01<TestType>();
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
	SECTION("Test circuit 02")
	{
		network = test_circuit_02<TestType>();
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
	SECTION("Test circuit 03")
	{
		network = test_circuit_03<TestType>();
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
	SECTION("Test circuit 04")
	{
		network = test_circuit_04<TestType>();
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
	SECTION("Test circuit 05")
	{
		network = test_circuit_05<TestType>();
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
	SECTION("Test circuit 06")
	{
		network = test_circuit_06<TestType>();
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
	SECTION("Test circuit 07")
	{
		network = test_circuit_07<TestType>();
		Device device = Device::path(network.num_qubits());
		std::vector<wire::Id> placement
		    = detail::hsat_placement(network, device);
		CHECK(placement_verify(device, placement));
	}
}
