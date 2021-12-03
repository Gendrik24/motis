#pragma once

#include <cinttypes>
#include <utility>

#include "cista/hashing.h"
#include "cista/offset_t.h"
#include "cista/reflection/comparable.h"

#include "utl/pipes.h"
#include "utl/to_vec.h"

#include "motis/string.h"
#include "motis/vector.h"

#include "motis/core/common/hash_helper.h"
#include "motis/core/common/unixtime.h"
#include "motis/core/schedule/edges.h"
#include "motis/core/schedule/nodes.h"

namespace motis {

struct primary_trip_id {
  CISTA_COMPARABLE()
  uint32_t station_id_{0U};
  uint32_t train_nr_{0U};
  mam_t first_departure_mam_{INVALID_MAM};
};

struct secondary_trip_id {
  CISTA_COMPARABLE();
  uint32_t target_station_id_{0U};
  mam_t last_arrival_mam_{INVALID_MAM};
  mcd::string line_id_;
};

struct trip_debug {
  std::string str() const {
    return file_ == nullptr ? ""
                            : static_cast<std::string>(*file_) + ":" +
                                  std::to_string(line_from_) + ":" +
                                  std::to_string(line_to_);
  }
  mcd::string* file_{nullptr};
  int line_from_{0}, line_to_{0};
};

struct full_trip_id {
  CISTA_COMPARABLE()
  primary_trip_id primary_;
  secondary_trip_id secondary_;
};

struct trip_info;

struct concrete_trip {
  CISTA_COMPARABLE()

  time get_first_dep_time() const;
  time get_last_arr_time() const;

  trip_info const* trp_;
  day_idx_t day_idx_;
};

struct trip_info {
  struct route_edge {
    route_edge() = default;

    route_edge(edge const* e) {  // NOLINT
      if (e != nullptr) {
        route_node_ = e->from_;
        for (auto i = 0U; i < route_node_->edges_.size(); ++i) {
          if (&route_node_->edges_[i] == e) {
            outgoing_edge_idx_ = i;
            return;
          }
        }
        assert(false);
      }
    }

#if defined(MOTIS_SCHEDULE_MODE_OFFSET)
    route_edge(ptr<edge const> e) : route_edge{e.get()} {}  // NOLINT
#endif

    friend bool operator==(route_edge const& a, route_edge const& b) {
      return std::tie(a.route_node_, a.outgoing_edge_idx_) ==
             std::tie(b.route_node_, b.outgoing_edge_idx_);
    }

    friend bool operator<(route_edge const& a, route_edge const& b) {
      return std::tie(a.route_node_, a.outgoing_edge_idx_) <
             std::tie(b.route_node_, b.outgoing_edge_idx_);
    }

    bool is_not_null() const { return route_node_ != nullptr; }

    edge* get_edge() const {
      assert(outgoing_edge_idx_ < route_node_->edges_.size());
      return &route_node_->edges_[outgoing_edge_idx_];
    }

    edge* operator->() const { return get_edge(); }

    operator edge*() const { return get_edge(); }  // NOLINT

    cista::hash_t hash() const {
      return cista::build_hash(route_node_ != nullptr ? route_node_->id_ : 0U,
                               outgoing_edge_idx_);
    }

    ptr<node> route_node_{nullptr};
    uint32_t outgoing_edge_idx_{0};
  };

  auto concrete_trips() const {
    return utl::iota(day_idx_t{0}, MAX_DAYS)  //
           | utl::remove_if([&](auto const day) {
               return !edges_->front()
                           ->m_.route_edge_.conns_.at(lcon_idx_)
                           .traffic_days_->test(day);
             })  //
           | utl::transform([&](auto const day) {
               return concrete_trip{this, day};
             })  //
           | utl::iterable();
  }

  full_trip_id id_;
  ptr<mcd::vector<route_edge> const> edges_{nullptr};
  mcd::vector<day_idx_t> day_offsets_;
  lcon_idx_t lcon_idx_{0U};
  trip_debug dbg_;
};

}  // namespace motis
