#pragma once

#include <string>
#include <vector>

#include "motis/module/message.h"

namespace nigiri {
struct timetable;
}

namespace motis::nigiri {

motis::module::msg_ptr route(std::vector<std::string> const& tags,
                             ::nigiri::timetable& tt,
                             motis::module::msg_ptr const& msg);

motis::module::msg_ptr route_mc_raptor(std::vector<std::string> const& tags,
                                        ::nigiri::timetable& tt,
                                        motis::module::msg_ptr const& msg,
                                        const bool use_bitsets);

}  // namespace motis::nigiri