#include "motis/paxmon/compact_journey_util.h"

#include <algorithm>

#include "motis/core/access/realtime_access.h"
#include "motis/core/access/trip_access.h"
#include "motis/core/access/trip_iterator.h"

namespace motis::paxmon {

std::optional<std::uint16_t> get_arrival_track(schedule const& sched,
                                               trip const* trp,
                                               std::uint32_t exit_station_id,
                                               motis::time exit_time) {
  if (trp == nullptr) {
    return {};
  }
  auto const sections = access::sections(trp);
  auto const section_it = std::find_if(
      begin(sections), end(sections), [&](access::trip_section const& sec) {
        return sec.to_station_id() == exit_station_id &&
               get_schedule_time(sched, sec.ev_key_to()) == exit_time;
      });
  if (section_it != end(sections)) {
    return (*section_it).lcon().full_con_->a_track_;
  } else {
    return {};
  }
}

std::optional<std::uint16_t> get_arrival_track(schedule const& sched,
                                               journey_leg const& leg) {
  return get_arrival_track(sched, get_trip(sched, leg.trip_idx_),
                           leg.exit_station_id_, leg.exit_time_);
}

std::optional<std::uint16_t> get_departure_track(schedule const& sched,
                                                 trip const* trp,
                                                 std::uint32_t enter_station_id,
                                                 motis::time enter_time) {
  if (trp == nullptr) {
    return {};
  }
  auto const sections = access::sections(trp);
  auto const section_it = std::find_if(
      begin(sections), end(sections), [&](access::trip_section const& sec) {
        return sec.from_station_id() == enter_station_id &&
               get_schedule_time(sched, sec.ev_key_from()) == enter_time;
      });
  if (section_it != end(sections)) {
    return (*section_it).lcon().full_con_->d_track_;
  } else {
    return {};
  }
}

std::optional<std::uint16_t> get_departure_track(schedule const& sched,
                                                 journey_leg const& leg) {
  return get_departure_track(sched, get_trip(sched, leg.trip_idx_),
                             leg.enter_station_id_, leg.enter_time_);
}

}  // namespace motis::paxmon
