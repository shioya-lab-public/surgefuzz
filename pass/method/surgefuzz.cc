#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/yosys.h"

#include "util.h"
#include <cassert>
#include <queue>
#include <tuple>

USING_YOSYS_NAMESPACE

struct SurgeFuzzPass : public Pass {
    CellTypes ct_ff, ct_mux, ct;
    CellTypes ct_cmp, ct_logic;

    enum class DATAFLOW_LEVEL { ALL, BIT, NONE };

    SurgeFuzzPass()
        : Pass("surgefuzz_cov_inst", "Instrument code for SurgeFuzz") {
        ct.setup();
        ct_ff.setup_internals_ff();
        for (auto type : std::vector<RTLIL::IdString>({ID($mux), ID($pmux)}))
            ct_mux.setup_type(type, {ID::A, ID::B, ID::S}, {ID::Y}, true);
    }

    struct DependentManager {
        struct DependentInfo {
            size_t depth;
            size_t reg_depth;
            bool is_ctrl;
            std::string cell_name;

            DependentInfo(size_t depth, size_t reg_depth, bool is_ctrl,
                          const std::string &cell_name)
                : depth(depth), reg_depth(reg_depth), is_ctrl(is_ctrl),
                  cell_name(cell_name) {}
            bool operator<(const DependentInfo &rhs) const {
                return depth < rhs.depth;
            }
        };

        // MODE mode;
        SigPool dependents;
        pool<SigSpec> dependet_pool;
        SigSet<DependentInfo, std::less<DependentInfo>> dependent_info;

        // DependentManager(MODE mode) : mode(mode){};
        DependentManager(){};
        void add(SigSpec sig, size_t depth, size_t reg_depth, bool is_ctrl,
                 const RTLIL::Cell *cell) {
            this->dependents.add(sig);
            this->dependet_pool.insert(sig);
            this->dependent_info.insert(
                sig,
                DependentInfo(depth, reg_depth, is_ctrl, cell->name.str()));
        }
        size_t size() { return this->dependents.size(); }
        void debug() {
            for (SigSpec sig : this->dependet_pool) {
                log("src: %s, size: %d\n", log_signal(sig), sig.size());
                for (const DependentInfo &u : this->dependent_info.find(sig)) {
                    log("\t%ld %ld %d\n", u.depth, u.reg_depth, u.is_ctrl);
                }
            }
        }
        SigPool get_dependets() { return this->dependents; }
        pool<SigSpec> &get_dependet_pool() { return this->dependet_pool; }
        DependentInfo get_dependent_info(SigSpec sig) {
            std::set<DependentInfo> ret = this->dependent_info.find(sig);
            return *(ret.begin());
        }
    };

    struct CircuitGraph {
        struct Node {
            const std::size_t id;
            const std::string name;
            const std::string type;
            std::string port_info;

            Node(std::size_t id, const RTLIL::Cell *cell)
                : id(id), name(cell->name.str()),
                  type(cell->type.str().substr(1)) {
                for (const auto &conn : cell->connections()) {
                    if (conn.first == ID::CLK) {
                        continue;
                    }
                    port_info += conn.first.str() + ":" +
                                 std::to_string(conn.second.size());
                }
            }

            std::string to_string() const {
                return std::to_string(id) + "," + name + "," + type + "," +
                       port_info;
            }

            bool operator<(const Node &rhs) const { return id < rhs.id; }
        };
        struct Edge {
            // (name_x) ->(edge_name)-> (name_y)
            const std::size_t id_x;
            const std::size_t id_y;
            const std::string id_x_port_name;
            const std::string id_y_port_name;
            const std::string name;
            const std::size_t width;

            Edge(std::size_t id_x, std::size_t id_y, const RTLIL::SigSpec &sig,
                 const RTLIL::IdString &id_x_port_name,
                 const RTLIL::IdString &id_y_port_name)
                : id_x(id_x), id_y(id_y),
                  id_x_port_name(id_x_port_name.str().substr(1)),
                  id_y_port_name(id_y_port_name.str().substr(1)),
                  name(log_signal(sig)), width(sig.size()) {}

            std::string to_string() const {
                return std::to_string(id_x) + "," + std::to_string(id_y) + "," +
                       id_x_port_name + "," + id_y_port_name + "," + name +
                       "," + std::to_string(width);
            }

            bool operator<(const Edge &rhs) const {
                return id_x < rhs.id_x ||
                       (id_x == rhs.id_x && id_y < rhs.id_y) ||
                       (id_x == rhs.id_x && id_y == rhs.id_y &&
                        id_x_port_name < rhs.id_x_port_name) ||
                       (id_x == rhs.id_x && id_y == rhs.id_y &&
                        id_x_port_name == rhs.id_x_port_name &&
                        id_y_port_name < rhs.id_y_port_name);
            }
        };

        std::set<Node> nodes;
        std::set<Edge> edges;
        std::map<std::string, std::size_t> name2id;

        void add_node(const RTLIL::Cell *cell) {
            std::string cell_name = cell->name.str();
            if (name2id.count(cell_name)) {
                nodes.insert(Node(name2id.at(cell_name), cell));
            } else {
                std::size_t new_id = name2id.size();
                name2id[cell_name] = new_id;
                nodes.insert(Node(new_id, cell));
            }
        }
        void add_edge(const RTLIL::Cell *cell_x, const RTLIL::Cell *cell_y,
                      const RTLIL::SigSpec sig,
                      const RTLIL::IdString &id_x_port_name,
                      const RTLIL::IdString &id_y_port_name) {
            std::size_t id_x = name2id[cell_x->name.str()];
            std::size_t id_y = name2id[cell_y->name.str()];
            edges.insert(Edge(id_x, id_y, sig, id_x_port_name, id_y_port_name));
        }

        std::string to_string() const {
            std::string str;
            str += "[node] name, type\n";
            for (const auto &node : nodes) {
                str += node.to_string() + "\n";
            }
            str += "[edge] node_name_x, node_name_y, edge_name\n";
            for (const auto &edge : edges) {
                str += edge.to_string() + "\n";
            }
            return str;
        }

        void write_node_csv(const std::string &filename) const {
            std::ofstream ofs(filename);
            if (!ofs) {
                std::cerr << "Failed to open file: " << filename << std::endl;
                exit(EXIT_FAILURE);
            }
            ofs << "id,name,type,port_info" << std::endl;
            for (const auto &node : nodes) {
                ofs << node.to_string() << std::endl;
            }
        }

        void write_edge_csv(const std::string &filename) const {
            std::ofstream ofs(filename);
            if (!ofs) {
                std::cerr << "Failed to open file: " << filename << std::endl;
                exit(EXIT_FAILURE);
            }
            ofs << "id_x,id_y,id_x_port_name,id_y_port_name,name,width"
                << std::endl;
            for (const auto &edge : edges) {
                ofs << edge.to_string() << std::endl;
            }
        }
    };

    DATAFLOW_LEVEL get_cell_dataflow_level(RTLIL::Cell *cell,
                                           RTLIL::IdString port_name) {
        assert(ct.cell_input(cell->type, port_name));

        if (cell->type.in(ID($not), ID($neg))) {
            if (port_name == ID::A) {
                assert(cell->getPort(ID::Y).size() ==
                       cell->getPort(port_name).size());
                return DATAFLOW_LEVEL::BIT;
            } else {
                assert(false);
            }
        }

        if (cell->type.in(ID($and), ID($or), ID($xor), ID($xnor))) {
            if (port_name == ID::A or port_name == ID::B) {
                assert(cell->getPort(ID::Y).size() >=
                       cell->getPort(port_name).size());
                return DATAFLOW_LEVEL::BIT;
            } else {
                assert(false);
            }
        }

        if (cell->type.in(ID($reduce_and), ID($reduce_or), ID($reduce_xor),
                          ID($reduce_xnor), ID($reduce_bool))) {
            if (port_name == ID::A) {
                assert(cell->getPort(ID::Y).size() == 1 and
                       cell->getPort(ID::Y).size() <=
                           cell->getPort(port_name).size());
                return DATAFLOW_LEVEL::ALL;
            } else {
                assert(false);
            }
        }

        if (cell->type.in(ID($lt), ID($le), ID($eq), ID($ne), ID($ge), ID($gt),
                          ID($logic_and), ID($logic_or), ID($logic_not))) {
            if (port_name == ID::A or port_name == ID::B) {
                return DATAFLOW_LEVEL::ALL;
            } else {
                assert(false);
            }
        }

        if (cell->type.in(ID($logic_not), ID($add), ID($sub), ID($mul),
                          ID($div), ID($shl), ID($shr), ID($shift),
                          ID($shiftx))) {
            if (port_name == ID::A or port_name == ID::B) {
                return DATAFLOW_LEVEL::BIT;
            } else {
                assert(false);
            }
        }

        if (cell->type.in(ID($mux))) {
            if (port_name == ID::A or port_name == ID::B) {
                assert(cell->getPort(ID::Y).size() ==
                       cell->getPort(port_name).size());
                return DATAFLOW_LEVEL::BIT;
            } else if (port_name == ID::S) {
                assert(cell->getPort(port_name).size() == 1);
                return DATAFLOW_LEVEL::ALL;
            } else {
                assert(false);
            }
        }

        if (cell->type.in(ID($dff), ID($dffe), ID($sdff), ID($sdffe), ID($adff),
                          ID($dlatch))) {
            if (port_name == ID::CLK || port_name == ID::SRST ||
                port_name == ID::ARST) {
                return DATAFLOW_LEVEL::NONE;
            } else if (port_name == ID::D) {
                assert(cell->getPort(ID::Q).size() ==
                       cell->getPort(port_name).size());
                return DATAFLOW_LEVEL::BIT;
            } else if (port_name == ID::EN) {
                return DATAFLOW_LEVEL::ALL;
            } else {
                assert(false);
            }
        }

        log_cell(cell);
        assert(false);
    }

    std::string get_module_name(const std::string &name) {
        std::smatch results;
        std::string module_name;
        if (std::regex_search(name, results,
                              std::regex("\\\\((\\w|\\.|\\[|\\])+)\\."))) {
            module_name = results[1].str();
        }
        return module_name;
    }

    int get_extract_offset(RTLIL::SigSpec sig, RTLIL::SigSpec wider_sig) {
        assert(sig.size() <= wider_sig.size());
        assert(sig.chunks().size() == 1);
        assert(wider_sig.extract(sig) == sig);

        for (int offset = 0; offset < wider_sig.size() - sig.size() + 1;
             offset++) {
            if (wider_sig.extract(offset, sig.size()) == sig) {
                return offset;
            }
        }
        __builtin_unreachable();
    }

    DependentManager search_dependents(RTLIL::Module *module,
                                       SigSet<RTLIL::Cell *> &drivers,
                                       RTLIL::SigSpec start_sig,
                                       size_t max_depth, size_t max_bit,
                                       const std::string &node_csv_filename,
                                       const std::string &edge_csv_filename) {
        struct State {
            const RTLIL::Cell *cell;
            const RTLIL::SigSpec sig;
            const RTLIL::IdString port_name;
            const std::size_t depth;
            const std::size_t reg_depth;
            const bool is_ctrl_path;

            State(const RTLIL::Cell *cell, RTLIL::SigSpec sig,
                  RTLIL::IdString port_name, std::size_t depth,
                  std::size_t reg_depth, bool is_ctrl_path)
                : cell(cell), sig(sig), port_name(port_name), depth(depth),
                  reg_depth(reg_depth), is_ctrl_path(is_ctrl_path) {}
        };

        SigMap sigmap(module);
        pool<RTLIL::SigSpec> visited_sigs;
        DependentManager dependent_manager;

        std::queue<State> queue;
        queue.push(
            State(nullptr, sigmap(start_sig), RTLIL::IdString(), 0, 0, false));

        CircuitGraph graph;

        while (not queue.empty() and dependent_manager.size() < max_bit) {
            State cur_state = queue.front();
            queue.pop();

            if (cur_state.depth >= max_depth) {
                continue;
            }

            RTLIL::SigSpec cur_sig = sigmap(cur_state.sig);
            {
                auto cur_sig_chunks = cur_sig.chunks();
                assert(cur_sig_chunks.size() == 1);
            }

            // List all cells to which cur_sig is connected.
            // When not present, it is outside the scope of the analysis.
            assert(drivers.has(sigmap(cur_sig)));
            std::set<RTLIL::Cell *> cells = drivers.find(sigmap(cur_sig));

            for (auto cell : cells) {
                // FIXME: Not supported cell
                if (cell->type == ID($memrd) || cell->type == ID($memrd_v2)) {
                    break;
                }

                const SigSpec output_sig = ct_ff.cell_known(cell->type)
                                               ? cell->getPort(ID::Q)
                                               : cell->getPort(ID::Y);

                // Check if cur_sig is included in the output of the cell.
                // If all bits of cur_sig are not included in the output of this
                // cell, then it is not a search target.
                if (output_sig.extract(cur_sig) != cur_sig) {
                    continue;
                }

                // When cur_signal is connected to a ff, this signal is selected
                // as a dependent signal.
                if (ct_ff.cell_known(cell->type) && cur_sig.size() <= 32) {
                    log("DETECT dependent ff of cur_sig: %s\n",
                        log_signal(cur_sig));
                    dependent_manager.add(cur_sig, cur_state.depth,
                                          cur_state.reg_depth,
                                          cur_state.is_ctrl_path, cell);
                }

                for (auto conn : cell->connections()) {
                    if (ct.cell_output(cell->type, conn.first)) {
                        continue;
                    }

                    RTLIL::IdString port_name = conn.first;
                    RTLIL::SigSpec next_sig = sigmap(conn.second);

                    for (const SigSpec next_sig_c : next_sig.chunks()) {
                        // Stop search at const value.
                        if (next_sig_c.is_fully_const()) {
                            continue;
                        }

                        // Stop search at signals already visited.
                        if (visited_sigs.count(next_sig_c)) {
                            continue;
                        }

                        // Decide how to reverse the flow
                        DATAFLOW_LEVEL df_level =
                            get_cell_dataflow_level(cell, port_name);

                        if (df_level == DATAFLOW_LEVEL::ALL) {
                            visited_sigs.insert(next_sig_c);

                            queue.push(State(
                                cell, next_sig_c, port_name,
                                cur_state.depth + 1,
                                cur_state.reg_depth +
                                    (ct_ff.cell_known(cell->type) ? 1 : 0),
                                cur_state.is_ctrl_path |
                                    (ct_mux.cell_known(cell->type) &&
                                     port_name == ID::S)));

                            graph.add_node(cell);
                            if (cur_state.cell != nullptr) {
                                graph.add_edge(cell, cur_state.cell, cur_sig,
                                               ct_ff.cell_known(cell->type)
                                                   ? ID::Q
                                                   : ID::Y,
                                               cur_state.port_name);
                            }
                        } else if (df_level == DATAFLOW_LEVEL::BIT) {
                            // Reverse trace only the necessary parts that are
                            // connected.
                            int offset =
                                get_extract_offset(cur_sig, output_sig);
                            if (next_sig_c.size() <= offset) {
                                continue;
                            }
                            RTLIL::SigSpec next_sig_chunk =
                                sigmap(next_sig_c.extract(
                                    offset,
                                    std::min(cur_sig.size(),
                                             next_sig_c.size() - offset)));
                            assert(!next_sig_chunk.is_fully_const());

                            if (visited_sigs.count(next_sig_chunk) == 0) {
                                // next_sig_chunk.remove_const();
                                visited_sigs.insert(next_sig_chunk);

                                queue.push(State(
                                    cell, next_sig_chunk, port_name,
                                    cur_state.depth + 1,
                                    cur_state.reg_depth +
                                        (ct_ff.cell_known(cell->type) ? 1 : 0),
                                    cur_state.is_ctrl_path |
                                        (ct_mux.cell_known(cell->type) &&
                                         port_name == ID::S)));

                                graph.add_node(cell);
                                if (cur_state.cell != nullptr) {
                                    graph.add_edge(
                                        cell, cur_state.cell, cur_sig,
                                        ct_ff.cell_known(cell->type) ? ID::Q
                                                                     : ID::Y,
                                        cur_state.port_name);
                                }
                            }
                        } else if (df_level == DATAFLOW_LEVEL::NONE) {
                            continue;
                        } else {
                            assert(false);
                        }
                    }
                }
            }
        }

        log("%s", graph.to_string().c_str());
        graph.write_node_csv(node_csv_filename);
        graph.write_edge_csv(edge_csv_filename);

        return dependent_manager;
    }

    SigSet<RTLIL::Cell *> setup_drivers(RTLIL::Module *module) {
        CellTypes ct;
        ct.setup();

        SigMap sigmap(module);
        SigSet<RTLIL::Cell *> drivers;

        for (auto cell : module->cells()) {
            // log_cell(cell);
            for (auto connection : cell->connections()) {
                // Get the signals on the port
                // (use sigmap to get a uniqe signal name)
                RTLIL::SigSpec sig = sigmap(connection.second);

                if (ct.cell_known(cell->type) and
                    ct.cell_input(cell->type, connection.first)) {
                    drivers.insert(sig, cell);
                }
                if (ct.cell_known(cell->type) and
                    ct.cell_output(cell->type, connection.first)) {
                    drivers.insert(sig, cell);
                }
            }
        }

        return drivers;
    }

    RTLIL::Wire *add_wire(RTLIL::Module *module, SigSpec sig,
                          std::string name) {
        RTLIL::Wire *inst = module->addWire(name, sig.size());
        // inst->port_output = true;
        module->connect(inst, sig);
        module->fixup_ports();
        return inst;
    }

    void instrument(RTLIL::Module *module, DependentManager &dependent_manager,
                    SigSpec target, std::string filename) {
        std::ofstream fs(filename);
        fs << "name,width,src,depth,reg_depth,is_ctrl,cell_name" << std::endl;

        SigSpec cov = SigSpec(0, 1);
        RTLIL::Wire *inst_cov = add_wire(module, cov, "\\coverage");
        fs << inst_cov->name.substr(1) << "," << inst_cov->width << ","
           << log_signal(cov) << ",0,0,0" << std::endl;
        log("name: %s, size: %d, src: %s\n", inst_cov->name.c_str(),
            inst_cov->width, log_signal(cov));

        RTLIL::Wire *inst_target =
            add_wire(module, target, "\\coverage_target");
        fs << inst_target->name.substr(1) << "," << inst_target->width << ","
           << log_signal(target) << ",0,0,0" << std::endl;
        log("name: %s, size: %d, src: %s\n", inst_target->name.c_str(),
            inst_target->width, log_signal(target));

        int dependent_idx = 0;
        for (auto dependent : dependent_manager.get_dependet_pool()) {
            RTLIL::Wire *inst_dependent =
                add_wire(module, dependent,
                         "\\dependent_" + std::to_string(dependent_idx++));
            fs << inst_dependent->name.substr(1) << "," << inst_dependent->width
               << "," << log_signal(dependent);
            auto info = dependent_manager.get_dependent_info(dependent);
            fs << "," << info.depth << "," << info.reg_depth << ","
               << info.is_ctrl << "," << info.cell_name << std::endl;
            log("name: %s, size: %d, src: %s\n", inst_dependent->name.c_str(),
                inst_dependent->width, log_signal(dependent));
        }

        // Instrument wire ancestores[32*N-1:0] = {dependentN-1, ... ,
        // dependent1, dependent0};
        RTLIL::SigSpec ancestors;
        {
            for (auto ancestor : dependent_manager.get_dependet_pool()) {
                log_assert(ancestor.size() <= 32);
                ancestor.extend_u0(32);
                ancestors.append(ancestor);
            }
        }
        RTLIL::Wire *ancestors_wire =
            module->addWire("\\fuzz_ancestors", ancestors.size());
        module->connect(ancestors_wire, ancestors);

        fs.close();
    }

    virtual void execute(std::vector<std::string> args, RTLIL::Design *design) {
        std::string inst_data_filename;
        std::string node_csv_filename;
        std::string edge_csv_filename;
        size_t max_bit = 0;
        for (size_t argidx = 1; argidx < args.size(); argidx++) {
            std::string arg = args[argidx];
            if (arg == "-w" && argidx + 1 < args.size()) {
                inst_data_filename = args[++argidx];
            } else if (arg == "-max_bit" && argidx + 1 < args.size()) {
                std::stringstream sstream(args[++argidx]);
                sstream >> max_bit;
            } else if (arg == "-node_csv" && argidx + 1 < args.size()) {
                node_csv_filename = args[++argidx];
            } else if (arg == "-edge_csv" && argidx + 1 < args.size()) {
                edge_csv_filename = args[++argidx];
            } else {
                std::cerr << "Failed to parse an unkown arg: " << args[argidx]
                          << std::endl;
                assert(false);
            }
        }
        assert(not inst_data_filename.empty());
        assert(max_bit != 0);

        for (auto module : design->selected_modules()) {
            SigSet<RTLIL::Cell *> drivers = setup_drivers(module);
            SigMap sigmap(module);

            SigSpec target_sig = get_annotated_signal(module);
            target_sig.remove_const();

            DependentManager dependent_manager =
                search_dependents(module, drivers, target_sig, 100, max_bit,
                                  node_csv_filename, edge_csv_filename);
            dependent_manager.debug();
            instrument(module, dependent_manager, target_sig,
                       inst_data_filename);
        }
    }
} SurgeFuzzPass;
