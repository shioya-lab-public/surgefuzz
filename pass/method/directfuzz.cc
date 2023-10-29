#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/yosys.h"

#include "util.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <map>
#include <queue>
#include <set>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

std::string extract_instance_name(const RTLIL::SigSpec &sig) {
    // Extract only the necessary part from the signal name
    std::string stripped_name;
    {
        const std::string name = log_signal(sig);
        const std::string::size_type start_pos = name.find("\\");
        const std::string::size_type end_pos = name.find("$", start_pos);

        if (start_pos != std::string::npos && end_pos != std::string::npos) {
            stripped_name = name.substr(start_pos, end_pos - start_pos);
        } else if (start_pos != std::string::npos) {
            stripped_name = name.substr(start_pos);
        } else {
            stripped_name = "";
        }
    }

    std::string module_name;
    {
        std::istringstream iss(stripped_name);
        std::vector<std::string> module_names;
        std::string token;
        while (std::getline(iss, token, '.')) {
            if (!token.empty()) {
                module_names.emplace_back(token);
            }
        }

        if (module_names.size() > 1) {
            module_name += module_names[0].substr(1);
            for (std::size_t i = 1; i < module_names.size() - 1; i++) {
                if (!module_name.empty()) {
                    module_name += "__";
                }
                module_name += module_names[i];
            }
        } else if (module_names.size() == 1) {
            module_name = "";
        }
    }
    return module_name;
}

class InstanCeconnectivityGraph {
  public:
    InstanCeconnectivityGraph(
        RTLIL::Module *module,
        SigSet<std::pair<RTLIL::Cell *, RTLIL::IdString>> &drivers,
        const std::string target_instance_name)
        : target_instance_name(target_instance_name) {
        ct.setup();
        ct_ff.setup_internals_ff();
        ct_mem.setup_type(ID($memrd), {ID::CLK, ID::EN, ID::ADDR}, {ID::DATA});
        ct_mem.setup_type(ID($memrd_v2),
                          {ID::CLK, ID::EN, ID::ARST, ID::SRST, ID::ADDR},
                          {ID::DATA});
        ct_mem.setup_type(ID($memwr), {ID::CLK, ID::EN, ID::ADDR, ID::DATA},
                          pool<RTLIL::IdString>());
        ct_mem.setup_type(ID($memwr_v2), {ID::CLK, ID::EN, ID::ADDR, ID::DATA},
                          pool<RTLIL::IdString>());
        ct_mem.setup_type(ID($meminit), {ID::ADDR, ID::DATA},
                          pool<RTLIL::IdString>());
        ct_mem.setup_type(ID($meminit_v2), {ID::ADDR, ID::DATA, ID::EN},
                          pool<RTLIL::IdString>());
        ct_mem.setup_type(ID($mem),
                          {ID::RD_CLK, ID::RD_EN, ID::RD_ADDR, ID::WR_CLK,
                           ID::WR_EN, ID::WR_ADDR, ID::WR_DATA},
                          {ID::RD_DATA});
        ct_mem.setup_type(ID($mem_v2),
                          {ID::RD_CLK, ID::RD_EN, ID::RD_ARST, ID::RD_SRST,
                           ID::RD_ADDR, ID::WR_CLK, ID::WR_EN, ID::WR_ADDR,
                           ID::WR_DATA},
                          {ID::RD_DATA});
        ct_assert.setup_type(ID($assert), {ID::A, ID::EN},
                             pool<RTLIL::IdString>(), true);

        construct(module, drivers);
        calc_distance(target_instance_name);
    }

    std::uint64_t get_distance(const std::string &x) const {
        if (distance.count(x) == 0) {
            log_warning("Failed to calculate the distance to module %s.",
                        x.c_str());
            return 256 /* TODO: dummy big value */;
        } else {
            std::uint64_t dist = distance.at(x);
            if (dist == std::numeric_limits<uint64_t>::max()) {
                log_warning("Failed to calculate the distance to module %s.",
                            x.c_str());
                return 256 /* TODO: dummy big value */;
            } else {
                return distance.at(x);
            }
        }
    }

    void generate_dot(const std::string &filepath) const {
        struct VertexProperty {
            std::string label;
        };
        using Graph = boost::adjacency_list<boost::listS, boost::vecS,
                                            boost::directedS, VertexProperty>;
        Graph g;
        std::map<std::string, Graph::vertex_descriptor> desc;
        {
            for (const auto &node : nodes) {
                desc[node] = boost::add_vertex(g);
                g[desc.at(node)].label = node;
            }
        }
        for (const auto &node_x : nodes) {
            if (edges.count(node_x) == 0) {
                continue;
            }
            for (const auto &node_y : edges.at(node_x)) {
                boost::add_edge(desc.at(node_x), desc.at(node_y), g);
            }
        }
        std::ofstream file(filepath);
        boost::write_graphviz(
            file, g, make_label_writer(get(&VertexProperty::label, g)));
    }

    void debug() const {
        log("----- Edge -----\n");
        for (const auto &node_x : nodes) {
            log("%s ->\n", node_x.c_str());
            if (edges.count(node_x) == 0) {
                continue;
            }
            for (const auto &node_y : edges.at(node_x)) {
                // node_x -> node_y
                log("\t%s\n", node_y.c_str());
            }
        }
        log("----- Distance -----\n");
        log("target: %s\n", target_instance_name.c_str());
        for (const auto &d : distance) {
            log("name(%s), distance(%lu)\n", d.first.c_str(), d.second);
        }
    }

  private:
    std::set<std::string> nodes;
    std::map<std::string, std::set<std::string>> edges;
    std::map<std::string, std::uint64_t> distance;
    const std::string target_instance_name;
    CellTypes ct_ff, ct_mem, ct_assert, ct;

    void construct(RTLIL::Module *module,
                   SigSet<std::pair<RTLIL::Cell *, RTLIL::IdString>> &drivers) {
        SigMap sigmap(module);
        for (auto cell : module->cells()) {
            if (not ct.cell_known(cell->type)) {
                continue;
            }
            // FIXME: Not supported cell
            if (ct_mem.cell_known(cell->type) ||
                ct_assert.cell_known(cell->type)) {
                continue;
            }
            const RTLIL::SigSpec start_sig =
                sigmap(ct_ff.cell_known(cell->type) ? cell->getPort(ID::Q)
                                                    : cell->getPort(ID::Y));

            for (auto start_sig_c : start_sig) {
                std::string instance_name = extract_instance_name(start_sig_c);
                std::set<std::string> adjacent_instance_names =
                    analyze_adjacent_instance_names(drivers, sigmap,
                                                    start_sig_c);
                for (const auto &adjacent_instance_name :
                     adjacent_instance_names) {
                    add(instance_name, adjacent_instance_name);
                }
            }
        }
    }

    void add(const std::string &x, const std::string &y) {
        nodes.insert(x);
        nodes.insert(y);
        if (edges.count(x) == 0) {
            edges[x] = std::set<std::string>();
        }
        edges[x].insert(y);
    }

    void calc_distance(const std::string &target) {
        for (const std::string &node : nodes) {
            distance[node] = std::numeric_limits<uint64_t>::max();
        }
        distance[target] = 0;

        std::queue<std::string> que;
        que.push(target);
        while (not que.empty()) {
            const std::string current = que.front();
            que.pop();
            if (edges.count(current) == 0) {
                continue;
            }
            for (const auto &next : edges.at(current)) {
                if (distance.at(current) + 1 < distance.at(next)) {
                    distance[next] = distance.at(current) + 1;
                    que.push(next);
                }
            }
        }
    }

    std::set<std::string> analyze_adjacent_instance_names(
        SigSet<std::pair<RTLIL::Cell *, RTLIL::IdString>> &drivers,
        const SigMap &sigmap, const RTLIL::SigSpec &start_sig) {
        pool<RTLIL::SigSpec> visited_sigs;
        const std::string start_instance_name =
            extract_instance_name(sigmap(start_sig));

        std::queue<RTLIL::SigSpec> queue;
        queue.push(start_sig);

        std::set<std::string> adjacent_instacen_names;
        while (not queue.empty()) {
            const RTLIL::SigSpec cur_sig = sigmap(queue.front());
            queue.pop();
            log_assert(cur_sig.chunks().size() == 1);

            const std::string cur_instance_name =
                extract_instance_name(cur_sig);

            if (start_instance_name != cur_instance_name) {
                adjacent_instacen_names.insert(cur_instance_name);
                continue;
            }

            const std::set<std::pair<RTLIL::Cell *, RTLIL::IdString>>
                cell_ports = drivers.find(cur_sig);

            for (auto cell_port : cell_ports) {
                RTLIL::Cell *cell = cell_port.first;
                RTLIL::IdString port_name = cell_port.second;

                // FIXME: Not supported cell
                if (ct_mem.cell_known(cell->type) ||
                    ct_assert.cell_known(cell->type)) {
                    continue;
                }

                const RTLIL::SigSpec next_sig =
                    sigmap(ct_ff.cell_known(cell->type) ? cell->getPort(ID::Q)
                                                        : cell->getPort(ID::Y));

                for (RTLIL::SigSpec next_sig_c : next_sig.chunks()) {
                    // Stop search at const value.
                    if (next_sig_c.is_fully_const()) {
                        continue;
                    }
                    // Stop search at signals already visited.
                    if (visited_sigs.count(next_sig_c)) {
                        continue;
                    }
                    visited_sigs.insert(next_sig_c);
                    queue.push(sigmap(next_sig_c));
                }
            }
        }
        return adjacent_instacen_names;
    }
};

class DirectedMuxCoverage {
  public:
    DirectedMuxCoverage(RTLIL::Module *module) {
        ct_mux.setup_type(ID($mux), {ID::A, ID::B, ID::S}, {ID::Y}, true);
        ct_pmux.setup_type(ID($pmux), {ID::A, ID::B, ID::S}, {ID::Y}, true);

        analyze_mux_select_signals(module);
    }

    void instrument(RTLIL::Module *module,
                    const InstanCeconnectivityGraph &graph,
                    const std::string &filepath) {
        std::ofstream fs(filepath);
        fs << "instance_name,coverage_signal_name,width,distance" << std::endl;

        for (auto m : unique_mux_select_signal_map) {
            const std::string instance_name = m.first;
            const pool<RTLIL::SigSpec> mux_select_signals = m.second;

            RTLIL::SigSpec coverage_sig;
            {
                for (auto sig : mux_select_signals) {
                    coverage_sig.append(sig);
                }
            }
            std::string sig_name = std::string("\\coverage_" + instance_name);
            RTLIL::Wire *coverage_wire =
                module->addWire(sig_name, coverage_sig.size());
            module->connect(coverage_wire, coverage_sig);
            fs << instance_name << "," << sig_name << ","
               << std::to_string(coverage_sig.size()) << ","
               << graph.get_distance(instance_name) << std::endl;
        }

        // Instrument target coverage signal
        RTLIL::SigSpec target_sig = get_annotated_signal(module);
        RTLIL::Wire *target_wire =
            module->addWire("\\coverage_target", target_sig.size());
        module->connect(target_wire, target_sig);

        module->fixup_ports();
    }

    void debug() {
        log("---- Mux Select Signals ----\n");
        std::size_t unique_mux = 0;
        std::size_t total_mux = 0;
        for (auto m : unique_mux_select_signal_map) {
            std::string module_name = m.first;
            std::string log_str;

            for (const RTLIL::SigSpec &sig : m.second) {
                log_str += "\t" + std::string(log_signal(sig)) + "\n";
            }
            log("module name: %s, unique mux: %ld, total mux: %ld\n",
                module_name.c_str(), m.second.size(),
                mux_select_signal_map.at(module_name).size());
            log("%s", log_str.c_str());

            unique_mux += m.second.size();
            total_mux += mux_select_signal_map.at(module_name).size();
        }
        log("[Overall] unique mux: %ld, total mux: %ld\n", unique_mux,
            total_mux);
    }

    // key: instance name, value: mux select signals in the key's module
    std::map<std::string, pool<RTLIL::SigSpec>> unique_mux_select_signal_map;
    std::map<std::string, std::vector<RTLIL::SigSpec>> mux_select_signal_map;

  private:
    CellTypes ct_mux, ct_pmux;

    void analyze_mux_select_signals(RTLIL::Module *module) {
        SigMap sigmap(module);
        for (auto cell : module->cells()) {
            if (ct_pmux.cell_known(cell->type)) {
                log_error("Found $pmux cell\n");
            }
            if (ct_mux.cell_known(cell->type)) {
                const RTLIL::SigSpec mux_select_signal =
                    sigmap(cell->getPort(ID::S));
                add(mux_select_signal,
                    extract_instance_name(mux_select_signal));
            }
        }
    }

    void add(const RTLIL::SigSpec &sig, const std::string &sig_modname) {
        log_assert(sig.size() == 1);
        if (not mux_select_signal_map.count(sig_modname)) {
            unique_mux_select_signal_map[sig_modname] = pool<RTLIL::SigSpec>();
            mux_select_signal_map[sig_modname] = std::vector<RTLIL::SigSpec>();
        }
        unique_mux_select_signal_map.at(sig_modname).insert(sig);
        mux_select_signal_map.at(sig_modname).emplace_back(sig);
    }
};

void setup_drivers(RTLIL::Module *module,
                   SigSet<std::pair<RTLIL::Cell *, RTLIL::IdString>> &drivers) {
    CellTypes ct;
    ct.setup();

    SigMap sigmap(module);

    for (auto cell : module->cells()) {
        for (auto connection : cell->connections()) {
            // Get the signals on the port
            // (use sigmap to get a uniqe signal name)
            RTLIL::SigSpec sig = sigmap(connection.second);

            if (ct.cell_known(cell->type) and
                ct.cell_input(cell->type, connection.first)) {
                drivers.insert(sig, std::make_pair(cell, connection.first));
            }
        }
    }
}

struct DirectFuzzPass : public Pass {
    DirectFuzzPass()
        : Pass("directfuzz_cov_inst", "Instrument code for DirectFuzz") {}

    void help() override {
        log("\n");
        log("\tdirectfuzz_cov_inst [--output_csv] [--output_dot] [--debug]\n");
        log("\n");
        log("This path instruments the code for DirectFuzz and outputs the "
            "instrumentation\n");
        log("information to a file. The path can also output an instance "
            "ceconnectivity graph.\n");
        log("\n");
        log("\t--output_csv filepath\n");
        log("\t\tfilepath to output the code instrumentation information\n");
        log("\n");
        log("\t--output_dot filepath\n");
        log("\t\tfilepath to output the instance ceconnectivity graph\n");
        log("\n");
        log("\t--debug\n");
        log("\t\tenable debug log\n");
        log("\n");
    }

    void execute(std::vector<std::string> args,
                 RTLIL::Design *design) override {
        log_header(design, "Executing directfuzz pass (instrument directed mux "
                           "toggle coverage).\n");
        log_push();

        std::string output_csv_filepath;
        std::string output_dot_filepath;
        bool debug = false;
        for (size_t argidx = 1; argidx < args.size(); argidx++) {
            std::string arg = args[argidx];
            if (arg == "--output_csv" && argidx + 1 < args.size() &&
                output_csv_filepath.empty()) {
                output_csv_filepath = args[++argidx];
            } else if (arg == "--output_dot" && argidx + 1 < args.size() &&
                       output_dot_filepath.empty()) {
                output_dot_filepath = args[++argidx];
            } else if (arg == "--debug") {
                debug = true;
            } else {
                log_error("Failed to parse an unkown arg: %s",
                          args[argidx].c_str());
            }
        }
        if (output_csv_filepath.empty()) {
            help();
            log_cmd_error("The option --output_csv must be specified.");
        }

        for (auto module : design->selected_modules()) {
            std::string target_instance_name;
            {
                RTLIL::SigSpec target_sig = get_annotated_signal(module);
                std::string target_instance_name =
                    extract_instance_name(target_sig);

                log("Target instance name: %s\n", target_instance_name.c_str());
            }

            SigSet<std::pair<RTLIL::Cell *, RTLIL::IdString>> drivers;
            setup_drivers(module, drivers);

            InstanCeconnectivityGraph graph(module, drivers,
                                            target_instance_name);
            if (debug) {
                graph.debug();
            }
            if (not output_dot_filepath.empty()) {
                graph.generate_dot(output_dot_filepath);
            }

            // Analyze mux select signals.
            DirectedMuxCoverage coverage(module);
            if (debug) {
                coverage.debug();
            }
            coverage.instrument(module, graph, output_csv_filepath);
        }

        log_pop();
    }
} DirectFuzzPass;

PRIVATE_NAMESPACE_END
