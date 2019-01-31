/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../networks/netlist.hpp"

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/esop.hpp>
#include <vector>

namespace tweedledum {

/*! \brief ESOP-phase synthesis.
 *
 * This is the in-place variant of ``esop_phase_synth``, in which the network
 * is passed as a parameter and can potentially already contain some gates.
 * The parameter ``qubits`` provides a qubit mapping to the existing qubits in
 * the network.
 *
 * \param network A quantum circuit
 * \param qubits A qubit mapping
 * \param function A Boolean function
 */
template<typename Network>
void esop_phase_synth(Network& network, std::vector<qubit_id> const& qubits,
                      kitty::dynamic_truth_table const& function)
{
	for (const auto& cube : kitty::esop_from_pprm(function)) {
		std::vector<qubit_id> controls;
		std::vector<qubit_id> targets;
		for (auto i = 0; i < function.num_vars(); ++i) {
			if (!cube.get_mask(i)) {
				continue;
			}
			assert(cube.get_bit(i));
			if (targets.empty()) {
				targets.emplace_back(qubits[i]);
			} else {
				controls.emplace_back(qubits[i]);
			}
		}
		if (!targets.empty()) {
			network.add_gate(gate::mcz, controls, targets);
		}
	}
}

/*! \brief ESOP-phase synthesis.
 *
 * Finds a quantum circuit using multiple-controlled Z gates that computes
 * a phase into a quantum state based on the Boolean function.  Note that
 * the circuit is the same for the function and its inverse.
 *
 * In order to find the multiple-controlled Z gates, the algorithm computes
 * the function's PPRM representation.
 *
 * \param function A Boolean function
 *
 * \algtype synthesis
 * \algexpects Boolean function
 * \algreturns Quantum circuit
 */
template<class Network>
Network esop_phase_synth(kitty::dynamic_truth_table const& function)
{
	Network network;
	const uint32_t num_qubits = function.num_vars();
	for (auto i = 0u; i < num_qubits; ++i) {
		network.add_qubit();
	}
	std::vector<qubit_id> qubits(num_qubits);
	std::iota(qubits.begin(), qubits.end(), 0u);
	esop_phase_synth(network, qubits, function);
	return network;
}

} // namespace tweedledum
