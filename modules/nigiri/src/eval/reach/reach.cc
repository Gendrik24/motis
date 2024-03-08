#include "motis/nigiri/eval/reach/commands.h"

#include <algorithm>
#include <string>
#include <fstream>

#include "conf/configuration.h"
#include "conf/options_parser.h"

#include "motis/bootstrap/import_settings.h"
#include "motis/bootstrap/dataset_settings.h"
#include "motis/bootstrap/motis_instance.h"
#include "motis/nigiri/nigiri.h"
#include "motis/core/common/date_time_util.h"
#include "nigiri/types.h"

#include "version.h"

using namespace motis;
using namespace motis::nigiri;
using namespace motis::bootstrap;
using namespace motis::module;
using namespace flatbuffers;

namespace motis::nigiri::eval {

struct reach_settings : public conf::configuration {
reach_settings() : configuration("Reach Options", "") {
  param(mode_, "mode", "Operation mode: Choose between create, extend, info ");
  param(start_time_, "start_time", "Start of considered time interval");
  param(end_time_, "end_time", "End of considered time interval");
  param(reach_store_idx_, "reach_store_id", "The id of a reach store");
  param(duration_, "duration", "The duration in minutes");
  param(n_threads_, "threads", "The number of threads used for computation");
}

unixtime get_start_time() {
  return parse_unix_time(start_time_, "%Y-%m-%d %H:%M %Z");
}

unixtime get_end_time() {
  return parse_unix_time(end_time_, "%Y-%m-%d %H:%M %Z");
}


std::string start_time_;
std::string end_time_;
std::string mode_{"create"};
std::string reach_store_idx_;
std::string duration_;
std::string n_threads_{"12"};
};

int reach(int argc, char const** argv) {
  dataset_settings dataset_opt;
  import_settings import_opt;
  reach_settings reach_opt;

  motis_instance instance;
  
  try {
    motis::nigiri::nigiri* n_ptr = nullptr;
    for (const auto module_ptr : instance.modules()) {
      if (module_ptr->name() == "Next Generation Routing") {
        n_ptr = dynamic_cast<motis::nigiri::nigiri*>(module_ptr);
      }
    }
    if (n_ptr == nullptr) {
      throw utl::fail("Required module_ptr not found!\n");
    }
    conf::options_parser parser({&dataset_opt, &import_opt, &reach_opt, n_ptr});
    parser.read_command_line_args(argc, argv, false);

    if (parser.help()) {
      std::cout << "\n\tmotis-reach (MOTIS v" << short_version()
                << ")\n\n";
      parser.print_help(std::cout);
      return 0;
    } else if (parser.version()) {
      std::cout << "motis-reach (MOTIS v" << long_version()
                << ")\n";
      return 0;
    }

    parser.read_configuration_file(true);
    parser.print_used(std::cout);
    parser.print_unrecognized(std::cout);

    utl::verify(reach_opt.mode_ == "create" || reach_opt.mode_ == "info" || reach_opt.mode_ == "extend",
                "The given operation mode is unknown.");

    utl::verify(reach_opt.mode_ != "create" ||
                      (!reach_opt.end_time_.empty() && !reach_opt.start_time_.empty()),
                "'start_time' and 'end_time' time must both be provided if mode is 'create'.");

    utl::verify(reach_opt.mode_ != "extend" ||
                    (!reach_opt.duration_.empty() && !reach_opt.reach_store_idx_.empty()),
                "'reach_store_id' and 'duration' time must both be provided if mode is 'extend'.");


  } catch (std::exception const& e) {
    std::cout << "options error: " << e.what() << "\n";
    return 1;
  }

  instance.import(module_settings{{"nigiri"}}, dataset_opt, import_opt);
  instance.init_modules(module_settings{{"nigiri"}});

  if (reach_opt.mode_ == "info") {
    for (const auto module_ptr : instance.modules()) {
      if  (module_ptr->name().compare("Next Generation Routing") != 0) {
         continue;
      }

      nigiri* n = dynamic_cast<nigiri*>(module_ptr);
      n->list_reach();
      return 0;
    }
  }

  message_creator fbb;
  if (reach_opt.mode_ == "extend") {
    fbb.create_and_finish(
        MsgContent::MsgContent_ReachExtendRequest,
        CreateReachExtendRequest(fbb,std::stoul(reach_opt.n_threads_),
                                 std::stoul(reach_opt.reach_store_idx_),
                                 std::stoi(reach_opt.duration_)
                                 ).Union(),
        "/nigiri/extend-reach");
  } else { // Case reach_opt.mode == "create"
    fbb.create_and_finish(
        MsgContent::MsgContent_ReachRequest,
        CreateReachRequest(fbb,std::stoul(reach_opt.n_threads_),
                           fbb.CreateString(reach_opt.start_time_),
                           fbb.CreateString(reach_opt.end_time_)
                           ).Union(),
        "/nigiri/build-reach");
  }

  instance.call(make_msg(fbb));
  return 0;
}

}

