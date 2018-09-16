/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-----------------------------------------------------------------------------*/
#pragma once

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "immutable_view.hpp"

#include <vector>
#include <iostream>

namespace tweedledum {

template<typename Network>
class slice_view : public immutable_view<Network> {
public:
	using gate_type = typename Network::gate_type;
	using node_type = typename Network::node_type;
	using node_ptr_type = typename Network::node_ptr_type;
	using storage_type = typename Network::storage_type;

	explicit slice_view(Network const& ntk)
	    : immutable_view<Network>(ntk)
	    , slices_(ntk)
	{
		update();
	}

	uint32_t depth() const
	{
		return depth_;
	}

	uint32_t level(node_type const& node) const
	{
		return slices_[node];
	}

	void update()
	{
		slices_.reset(0);
		compute_slices();
		this->clear_marks();
	}

private:
	uint32_t compute_slices(node_type const& node)
	{
		if (this->mark(node)) {
			return slices_[node];
		}

		if (node.gate.is(gate_kinds_t::input)) {
			return slices_[node] = 0;
		}

		uint32_t slice = 0;
		auto choices = this->get_predecessor_choices(node);
		for (auto choice_index : choices) {
			slice = std::max(slice, compute_slices(this->get_node(choice_index)));
		}
		this->mark(node, 1);
		for (auto choice_index : choices) {
			slices_[choice_index] = slice;
		}
		return slices_[node] = slice + 1;
	}

	void compute_slices()
	{
		depth_ = 0;
		this->foreach_output( [&](auto const& node) {
			depth_ = std::max(depth_, compute_slices(node));
		});
	}

private:
	node_map<uint32_t, Network> slices_;
	uint32_t depth_;
};

} // namespace tweedledum
