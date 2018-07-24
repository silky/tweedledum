/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-----------------------------------------------------------------------------*/
#pragma once

#include "detail/foreach.hpp"
#include "detail/storage.hpp"
#include "gates/gate_kinds.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

namespace tweedledum {

/*! Directed acyclic graph (dag) path representation.
 *
 * Represent a quantum circuit as a directed acyclic graph. The nodes in the
 * graph are either input/output nodes or operation nodes. All nodes store
 * a gate object, which is defined as a class template parameter, which allows
 * great flexibility in the types supported as gates.
 *
 * Path DAG: the edges encode only the input/output relationship between the
 * gates. That is, a directed edge from node A to node B means that the qubit
 * _must_ pass from the output of A to the input of B.
 *
 * Some natural properties like depth can be computed directly from the graph.
 */
template<typename G>
struct dag_path {
public:
	using gate_type = G;
	using node_type = uniform_node<gate_type, 1, 1>;
	using node_ptr_type = typename node_type::pointer_type;
	using storage_type = storage<node_type>;

public:
	dag_path()
	    : storage_(std::make_shared<storage_type>())
	{}

	dag_path(std::shared_ptr<storage_type> storage)
	    : storage_(storage)
	{}

	uint32_t size() const
	{
		return storage_->nodes.size() + storage_->outputs.size();
	}

	uint32_t num_qubits() const
	{
		return storage_->inputs.size();
	}

	node_type& get_node(uint32_t index)
	{
		return storage_->nodes[index];
	}

	std::vector<node_ptr_type> get_children(node_type const& node, uint32_t qubit_id)
	{
		std::vector<node_ptr_type> ret;
		auto input_id = node.gate.get_input_id(qubit_id);
		auto child = node.qubit[input_id][0];
		auto child_node = get_node(child.index);
		while (not node.gate.is_dependent(child_node.gate)) {
			ret.emplace_back(child);
			input_id = child_node.gate.get_input_id(qubit_id);
			child = child_node.qubit[input_id][0];
			child_node = get_node(child.index);
		}
		ret.emplace_back(child);
		return ret;
	}

	void add_qubit(std::string const& qubit)
	{
		auto qubit_id = create_qubit();
		label_to_id_.emplace(qubit, qubit_id);
		id_to_label_.emplace_back(qubit);
	}

	void mark_as_input(std::string const& qubit)
	{
		std::cout << "Mark as input: " << qubit << '\n';
	}

	void mark_as_output(std::string const& qubit)
	{
		std::cout << "Mark as output: " << qubit << '\n';
	}

	void add_gate(gate_kinds_t kind, std::string const& target)
	{
		auto qubit_id = label_to_id_[target];
		add_gate(kind, qubit_id);
	}

	void add_gate(gate_kinds_t kind, uint32_t target_id)
	{
		gate_type single_target_gate(kind, target_id);
		do_add_gate(single_target_gate);
	}

	void add_controlled_gate(gate_kinds_t kind, std::string const& control,
	                         std::string const& target)
	{
		auto target_id = label_to_id_[target];
		auto control_id = label_to_id_[control];
		add_controlled_gate(kind, control_id, target_id);
	}

	void add_controlled_gate(gate_kinds_t kind, uint32_t control_id,
	                         uint32_t target_id)
	{
		gate_type controlled_gate(kind, target_id, control_id);
		do_add_gate(controlled_gate);
	}

	void add_multiple_controlled_gate(gate_kinds_t kind,
	                                  std::vector<std::string> const& labels)
	{
		std::vector<uint32_t> qubits;
		for (auto& label : labels) {
			qubits.push_back(label_to_id_[label]);
		}
		add_multiple_controlled_gate(kind, qubits);
	}

	void add_multiple_controlled_gate(gate_kinds_t kind,
	                                  std::vector<uint32_t> const& qubits)
	{
		gate_type multiple_controlled_gate(kind, qubits[2], qubits[0],
		                                   qubits[1]);
		do_add_gate(multiple_controlled_gate);
	}

	template<typename Fn>
	void foreach_qubit(Fn&& fn) const
	{
		auto index = 0u;
		for (auto& label : id_to_label_) {
			fn(index++, label);
		}
	}

	template<typename Fn>
	void foreach_node(Fn&& fn) const
	{
		detail::foreach_element(storage_->nodes.cbegin(),
		                        storage_->nodes.cend(), fn);
		detail::foreach_element(storage_->outputs.cbegin(),
		                        storage_->outputs.cend(), fn,
		                        storage_->nodes.size());
	}

	template<typename Fn>
	void foreach_input(Fn&& fn) const
	{
		for (auto arc : storage_->inputs) {
			fn(storage_->nodes[arc.index], arc.index);
		}
	}

	template<typename Fn>
	void foreach_output(Fn&& fn) const
	{
		uint32_t index = storage_->nodes.size();
		detail::foreach_element(storage_->outputs.cbegin(),
		                        storage_->outputs.cend(), fn, index);
	}

	template<typename Fn>
	void foreach_gate(Fn&& fn) const
	{
		uint32_t index = storage_->inputs.size();
		auto begin = storage_->nodes.cbegin() + index;
		auto end = storage_->nodes.cend();
		detail::foreach_element(begin, end, fn, index);
	}

	template<typename Fn>
	void foreach_child(node_type& n, Fn&& fn) const
	{
		static_assert(
		    detail::is_callable_without_index_v<
		        Fn, node_ptr_type,
		        void> || detail::is_callable_with_index_v<Fn, node_ptr_type, void>);

		for (auto qubit_id = 0u; qubit_id < n.qubit.size(); ++qubit_id) {
			if (n.qubit[qubit_id][0] == node_ptr_type::max) {
				continue;
			}
			if constexpr (detail::is_callable_without_index_v<
			                  Fn, node_ptr_type, void>) {
				fn(n.qubit[qubit_id][0]);
			} else if constexpr (detail::is_callable_with_index_v<
			                         Fn, node_ptr_type, void>) {
				fn(n.qubit[qubit_id][0], qubit_id);
			}
		}
	}

	template<typename Fn>
	void foreach_child(node_type& n, uint32_t qubit_id, Fn&& fn) const
	{
		auto index = n.gate.get_input_id(qubit_id);
		if (n.qubit[index][0] == node_ptr_type::max) {
			return;
		}
		fn(n.qubit[index][0]);
	}

	template<typename Fn>
	void foreach_node(Fn&& fn)
	{
		detail::foreach_element(storage_->nodes.begin(),
		                        storage_->nodes.end(), fn);
		detail::foreach_element(storage_->outputs.begin(),
		                        storage_->outputs.end(), fn,
		                        storage_->nodes.size());
	}

	template<typename Fn>
	void foreach_gate(Fn&& fn)
	{
		uint32_t index = storage_->inputs.size();
		auto begin = storage_->nodes.begin() + index;
		auto end = storage_->nodes.end();
		detail::foreach_element(begin, end, fn, index);
	}

	template<typename Fn>
	void foreach_child(node_type& n, Fn&& fn)
	{
		static_assert(
		    detail::is_callable_without_index_v<
		        Fn, node_ptr_type,
		        void> || detail::is_callable_with_index_v<Fn, node_ptr_type, void>);

		for (auto qubit_id = 0u; qubit_id < n.qubit.size(); ++qubit_id) {
			if (n.qubit[qubit_id][0] == node_ptr_type::max) {
				continue;
			}
			if constexpr (detail::is_callable_without_index_v<
			                  Fn, node_ptr_type, void>) {
				fn(n.qubit[qubit_id][0]);
			} else if constexpr (detail::is_callable_with_index_v<
			                         Fn, node_ptr_type, void>) {
				fn(n.qubit[qubit_id][0], qubit_id);
			}
		}
	}

	template<typename Fn>
	void foreach_child(node_type& n, uint32_t qubit_id, Fn&& fn)
	{
		auto index = n.gate.get_input_id(qubit_id);
		if (n.qubit[index][0] == node_ptr_type::max) {
			return;
		}
		fn(n.qubit[index][0]);
	}

	void clear_marks()
	{
		std::for_each(storage_->nodes.begin(), storage_->nodes.end(),
		              [](auto& n) { n.data[0].b0 = 0; });
	}

	auto mark(node_type& n)
	{
		return n.data[0].b0;
	}

	void mark(node_type& n, std::uint8_t value)
	{
		n.data[0].b0 = value;
	}

	void remove_marked_nodes()
	{
		auto old_storage = storage_;
		storage_
		    = std::make_shared<storage_type>(old_storage->nodes.size());
		for (auto i = 0u; i < old_storage->inputs.size(); ++i) {
			create_qubit();
		}
		for (auto& node : old_storage->nodes) {
			if (node.gate.is(gate_kinds_t::input) || mark(node)) {
				continue;
			}
			do_add_gate(node.gate);
		}
	}

private:
	uint32_t create_qubit()
	{
		// std::cout << "Create qubit\n";
		auto qubit_id = static_cast<uint32_t>(storage_->inputs.size());
		auto index = static_cast<uint32_t>(storage_->nodes.size());

		// Create input node
		auto& input_node = storage_->nodes.emplace_back();
		input_node.gate.kind(gate_kinds_t::input);
		input_node.gate.target_qubit(qubit_id);
		storage_->inputs.emplace_back(index, false);

		// Create ouput node
		auto& output_node = storage_->outputs.emplace_back();
		output_node.gate.kind(gate_kinds_t::output);
		output_node.gate.target_qubit(qubit_id);
		output_node.qubit[0][0] = {index, true};

		// std::cout << "[done]\n";
		return qubit_id;
	}

	void do_add_gate(gate_type gate)
	{
		auto node_index = storage_->nodes.size();
		auto& node = storage_->nodes.emplace_back();

		node.gate = gate;
		node.gate.foreach_control(
		    [&](auto qubit_id) { connect_node(qubit_id, node_index); });
		node.gate.foreach_target(
		    [&](auto qubit_id) { connect_node(qubit_id, node_index); });
	}

	void connect_node(uint32_t qubit_id, uint32_t node_index)
	{
		assert(storage_->outputs[qubit_id].qubit[1].size() > 0);
		auto& node = storage_->nodes.at(node_index);
		auto& output = storage_->outputs.at(qubit_id);

		// std::cout << "[" << qubit_id << ":" << node_index
		//           << "] Connecting node ("
		//           << gate_name(node.gate.kind()) << ")\n";

		auto connector = node.gate.get_input_id(qubit_id);
		foreach_child(output, [&node, connector](auto arc) {
			node.qubit[connector][0] = arc;
		});
		output.qubit[0][0] = {node_index, true};
		return;
	}

private:
	std::unordered_map<std::string, uint32_t> label_to_id_;
	std::vector<std::string> id_to_label_;
	std::shared_ptr<storage_type> storage_;
};

} // namespace tweedledum