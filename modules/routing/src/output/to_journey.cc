#include "motis/routing/output/to_journey.h"

#include <string>

#include "motis/core/common/interval_map.h"
#include "motis/core/access/service_access.h"
#include "motis/core/access/time_access.h"
#include "motis/core/journey/generate_journey_transport.h"

namespace motis::routing::output {

std::vector<journey::transport> generate_journey_transports(
    std::vector<intermediate::transport> const& transports,
    schedule const& sched) {
  struct con_info_cmp {
    bool operator()(connection_info const* a, connection_info const* b) const {
      auto train_nr_a = output_train_nr(a->train_nr_, a->original_train_nr_);
      auto train_nr_b = output_train_nr(b->train_nr_, b->original_train_nr_);
      return std::tie(a->line_identifier_, a->category_, train_nr_a, a->dir_) <
             std::tie(b->line_identifier_, b->category_, train_nr_b, b->dir_);
    }
  };

  std::vector<journey::transport> journey_transports;
  interval_map<connection_info const*, con_info_cmp> intervals;
  for (auto const& t : transports) {
    if (t.con_ != nullptr) {
      auto con_info = t.con_->full_con_->con_info_;
      while (con_info != nullptr) {
        intervals.add_entry(con_info, t.from_, t.to_);
        con_info = con_info->merged_with_;
      }
    } else {
      journey_transports.push_back(generate_journey_transport(
          t.from_, t.to_, nullptr, sched, t.duration_, t.mumo_id_,
          t.mumo_price_, t.mumo_accessibility_));
    }
  }

  for (auto const& t : intervals.get_attribute_ranges()) {
    for (auto const& range : t.second) {
      journey_transports.push_back(
          generate_journey_transport(range.from(), range.to(), t.first, sched));
    }
  }

  std::sort(begin(journey_transports), end(journey_transports),
            [](journey::transport const& lhs, journey::transport const& rhs) {
              return lhs.from_ < rhs.from_;
            });

  return journey_transports;
}

std::vector<journey::trip> generate_journey_trips(
    std::vector<intermediate::transport> const& transports,
    schedule const& sched) {
  struct trp_cmp {
    bool operator()(concrete_trip const& a, concrete_trip const& b) const {
      return a.day_idx_ == b.day_idx_ && a.trp_->id_ < b.trp_->id_;
    }
  };

  auto trip_intervals = interval_map<concrete_trip, trp_cmp>{};
  for (auto const& t : transports) {
    if (t.con_ == nullptr) {
      continue;
    }

    for (auto const& trp : *sched.merged_trips_.at(t.con_->trips_)) {
      trip_intervals.add_entry(concrete_trip{trp, t.day_}, t.from_, t.to_);
    }
  }

  std::vector<journey::trip> journey_trips;
  for (auto const& [ctrp, ranges] : trip_intervals.get_attribute_ranges()) {
    auto const& p = ctrp.trp_->id_.primary_;
    auto const& s = ctrp.trp_->id_.secondary_;
    for (auto const& range : ranges) {
      journey_trips.push_back(journey::trip{
          static_cast<unsigned>(range.from()),
          static_cast<unsigned>(range.to()),
          extern_trip{sched.stations_.at(p.station_id_)->eva_nr_, p.train_nr(),
                      motis_to_unixtime(sched, time{}),
                      sched.stations_.at(s.target_station_id_)->eva_nr_,
                      motis_to_unixtime(sched, ctrp.get_last_arr_time()),
                      s.line_id_},
          ctrp.trp_->dbg_.str()});
    }
  }

  std::sort(begin(journey_trips), end(journey_trips),
            [](journey::trip const& lhs, journey::trip const& rhs) {
              return lhs.from_ < rhs.from_;
            });

  return journey_trips;
}

std::vector<journey::stop> generate_journey_stops(
    std::vector<intermediate::stop> const& stops, schedule const& sched) {
  std::vector<journey::stop> journey_stops;
  for (auto const& stop : stops) {
    auto const& station = *sched.stations_[stop.station_id_];
    journey_stops.push_back(
        {stop.exit_, stop.enter_, station.name_.str(), station.eva_nr_.str(),
         station.width_, station.length_,
         stop.a_time_ != INVALID_TIME
             ? journey::stop::event_info{true,
                                         motis_to_unixtime(
                                             sched.schedule_begin_,
                                             stop.a_time_),
                                         motis_to_unixtime(
                                             sched.schedule_begin_,
                                             stop.a_sched_time_),
                                         stop.a_reason_,
                                         stop.a_track_ != nullptr
                                             ? stop.a_track_->str()
                                             : "",
                                         stop.a_sched_track_ != nullptr
                                             ? stop.a_sched_track_->str()
                                             : ""}
             : journey::stop::event_info{false, 0, 0,
                                         timestamp_reason::SCHEDULE, "", ""},
         stop.d_time_ != INVALID_TIME
             ? journey::stop::event_info{true,
                                         motis_to_unixtime(
                                             sched.schedule_begin_,
                                             stop.d_time_),
                                         motis_to_unixtime(
                                             sched.schedule_begin_,
                                             stop.d_sched_time_),
                                         stop.d_reason_,
                                         stop.d_track_ != nullptr
                                             ? stop.d_track_->str()
                                             : "",
                                         stop.d_sched_track_ != nullptr
                                             ? stop.d_sched_track_->str()
                                             : ""}
             : journey::stop::event_info{false, 0, 0,
                                         timestamp_reason::SCHEDULE, "", ""}});
  }
  return journey_stops;
}

std::vector<journey::ranged_attribute> generate_journey_attributes(
    std::vector<intermediate::transport> const& transports) {
  interval_map<attribute const*> attributes;
  for (auto const& t : transports) {
    if (t.con_ == nullptr) {
      continue;
    } else {
      for (auto const& attr :
           t.con_->full_con_->con_info_->attributes(t.day_)) {
        attributes.add_entry(attr, t.from_, t.to_);
      }
    }
  }

  std::vector<journey::ranged_attribute> journey_attributes;
  for (auto const& attribute_range : attributes.get_attribute_ranges()) {
    auto const& attribute = attribute_range.first;
    auto const& attribute_ranges = attribute_range.second;
    auto const& code = attribute->code_;
    auto const& text = attribute->text_;

    for (auto const& range : attribute_ranges) {
      journey_attributes.push_back({static_cast<unsigned>(range.from()),
                                    static_cast<unsigned>(range.to_),
                                    {code, text}});
    }
  }

  return journey_attributes;
}

}  // namespace motis::routing::output
