#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "geo/latlng.h"

#include "motis/core/schedule/time.h"
#include "motis/core/statistics/statistics.h"
#include "motis/intermodal/mumo_type.h"
#include "motis/intermodal/ppr_profiles.h"
#include "motis/intermodal/ridesharing_edges.h"
#include "motis/protocol/Message_generated.h"

namespace motis::intermodal {

struct car_parking_edge {
  int32_t parking_id_{};
  geo::latlng parking_pos_{};
  uint16_t car_duration_{};
  uint16_t foot_duration_{};
  uint16_t foot_accessibility_{};
  uint16_t total_duration_{};
  bool uses_car_{};
};

struct ridesharing_edge {
  std::string lift_key_{};
  std::time_t t_{};
  uint16_t price_{};
  uint16_t from_leg_{};
  uint16_t to_leg_{};
  geo::latlng from_loc_{};
  geo::latlng to_loc_{};
};

struct mumo_edge {
  mumo_edge(std::string from, std::string to, geo::latlng const& from_pos,
            geo::latlng const& to_pos, duration const d, uint16_t accessibility,
            mumo_type const type, int const id)
      : from_(std::move(from)),
        to_(std::move(to)),
        from_pos_(from_pos),
        to_pos_(to_pos),
        duration_(d),
        accessibility_(accessibility),
        type_(type),
        id_(id) {}

  std::string from_, to_;
  geo::latlng from_pos_, to_pos_;
  duration duration_;
  uint16_t accessibility_;
  mumo_type type_;
  int id_;
  std::optional<ridesharing_edge> ridesharing_;
  std::optional<car_parking_edge> car_parking_;
};

using appender_fun = std::function<mumo_edge&(
    std::string const&, geo::latlng const&, duration const, uint16_t const,
    mumo_type const, int const)>;

using mumo_stats_appender_fun = std::function<void(stats_category&&)>;

void make_ridesharing_request(
    ridesharing_edges& rs_edges, geo::latlng const& start,
    geo::latlng const& dest, bool start_is_intermodal, bool dest_is_intermodal,
    std::time_t t,
    std::pair<uint16_t, motis::ppr::SearchOptions const*> mode_data);
void make_starts(IntermodalRoutingRequest const*, geo::latlng const&,
                 appender_fun const&, mumo_stats_appender_fun const&,
                 ppr_profiles const&);
void make_dests(IntermodalRoutingRequest const*, geo::latlng const&,
                appender_fun const&, mumo_stats_appender_fun const&,
                ppr_profiles const&);

void remove_intersection(std::vector<mumo_edge>& starts,
                         std::vector<mumo_edge>& destinations,
                         geo::latlng const& query_start,
                         geo::latlng const& query_destination,
                         routing::SearchDir);

std::vector<flatbuffers::Offset<routing::AdditionalEdgeWrapper>> write_edges(
    flatbuffers::FlatBufferBuilder& fbb,  //
    std::vector<mumo_edge> const& starts,
    std::vector<mumo_edge> const& destinations,
    ridesharing_edges const& rs_edges,
    std::vector<mumo_edge const*>& edge_mapping);

}  // namespace motis::intermodal
