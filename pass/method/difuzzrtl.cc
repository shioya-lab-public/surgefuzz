#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/yosys.h"

#include "util.h"
#include <cassert>
#include <map>
#include <queue>
#include <set>

USING_YOSYS_NAMESPACE

struct DifuzzRTLPass : public Pass {
    CellTypes ct_ff, ct_mux, ct_mem, ct_assert, ct;
    bool sv2v_mode = true;

    DifuzzRTLPass()
        : Pass("difuzzrtl_cov_inst", "Instrument code for DifuzzRTL") {
        ct.setup();
        ct_ff.setup_internals_ff();

        for (auto type : std::vector<RTLIL::IdString>({ID($mux), ID($pmux)}))
            ct_mux.setup_type(type, {ID::A, ID::B, ID::S}, {ID::Y}, true);

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
    }

    std::string extract_modname_from_sig(const RTLIL::SigSpec sig) {
        // Extract only the necessary part from the signal name
        std::string stripped_name;
        {
            const std::string name = log_signal(sig);
            const std::string::size_type start_pos = name.find("\\");
            const std::string::size_type end_pos = name.find("$", start_pos);

            if (start_pos != std::string::npos &&
                end_pos != std::string::npos) {
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
                module_name += module_names[0];
            }
        }
        return module_name;
    }

    bool is_control_register(
        SigSet<std::pair<RTLIL::Cell *, RTLIL::IdString>> &drivers,
        const SigMap &sigmap, RTLIL::SigSpec start_sig) {
        pool<RTLIL::SigSpec> visited_sigs;
        const std::string start_sig_modname =
            extract_modname_from_sig(sigmap(start_sig));

        std::queue<RTLIL::SigSpec> queue;
        queue.push(start_sig);

        while (not queue.empty()) {
            const RTLIL::SigSpec cur_sig = sigmap(queue.front());
            queue.pop();
            assert(cur_sig.chunks().size() == 1);

            const std::string cur_sig_modname =
                extract_modname_from_sig(cur_sig);

            // Search only within the same module.
            if (cur_sig_modname != "" and
                start_sig_modname != cur_sig_modname) {
                continue;
            }

            // List all cells to which cur_sig is connected.
            // When not present, it is outside the scope of the analysis.
            // assert (drivers.has(sigmap(cur_sig)));

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

                // When cur_signal is connected to the selection signal of the
                // mux, src_cell is a control register.
                if (ct_mux.cell_known(cell->type) && port_name == ID::S) {
                    return true;
                }

                // A contol register is connected to the selection signal of the
                // mux directory.
                if (ct_ff.cell_known(cell->type)) {
                    return false;
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
        return false;
    }

    class ControlReg {
      public:
        ControlReg(const RTLIL::Cell *cell, const RTLIL::SigSpec sig)
            : cell(cell), sig(sig) {}
        int get_width() const { return sig.size(); }
        RTLIL::SigSpec get_sig() const { return sig; }
        std::string to_string() const {
            std::string ret;
            ret += "cell(" + std::string(cell->name.str()) + "), ";
            ret += "sig(" + std::string(log_signal(sig)) + "), ";
            ret += "size(" + std::to_string(sig.size()) + ")";
            return ret;
        }

      private:
        const RTLIL::Cell *cell;
        const RTLIL::SigSpec sig;
    };

    class ControlRegManager {
      public:
        void add(const RTLIL::Cell *cell, RTLIL::SigSpec sig,
                 const std::string &sig_modname) {
            if (not control_reg_map.count(sig_modname)) {
                control_reg_map[sig_modname] = std::vector<ControlReg>();
            }
            control_reg_map.at(sig_modname).emplace_back(ControlReg(cell, sig));
        }

        void log_stat() {
            log("---- Control Registers ----\n");
            int total_bits = 0;
            for (auto m : control_reg_map) {
                std::string module_name = m.first;
                int total_bits_in_module = 0;
                std::string reg_str;

                for (const ControlReg &reg : m.second) {
                    reg_str += "\t" + reg.to_string() + "\n";
                    total_bits_in_module += reg.get_width();
                }

                log("module name: %s, total bits: %d\n", module_name.c_str(),
                    total_bits_in_module);
                log("%s", reg_str.c_str());
                total_bits += total_bits_in_module;
            }
            log("total bits: %d\n", total_bits);
            log("--------------------------------\n");
        }

        std::map<std::string, std::vector<ControlReg>> get_control_reg_map() {
            return control_reg_map;
        }

      private:
        // key: module name, value: control registers in the key's module
        std::map<std::string, std::vector<ControlReg>> control_reg_map;
    };

    void search_control_registers(
        RTLIL::Module *module,
        SigSet<std::pair<RTLIL::Cell *, RTLIL::IdString>> &drivers,
        ControlRegManager &control_reg_manager) {
        SigMap sigmap(module);

        for (auto cell : module->cells()) {
            // Check if the cell's type is Register.
            if (not ct_ff.cell_known(cell->type)) {
                continue;
            }

            // Register's output signal
            const SigSpec sig = sigmap(cell->getPort(ID::Q));

            for (const auto &sig_c : sig.chunks()) {
                if (is_control_register(drivers, sigmap, sig_c)) {
                    const std::string sig_c_modname =
                        extract_modname_from_sig(sig_c);
                    log_debug("Detected a control register: name(%s) "
                              "signame(%s) modname(%s)\n",
                              cell->name.c_str(), log_signal(sig_c),
                              sig_c_modname.c_str());
                    control_reg_manager.add(cell, sig_c, sig_c_modname);
                }
            }
        }
    }

    void
    setup_drivers(RTLIL::Module *module,
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
                if (ct.cell_known(cell->type) and
                    ct.cell_output(cell->type, connection.first)) {
                }
            }
        }
    }

    void instrument(RTLIL::Module *module, const std::string &module_name,
                    const vector<ControlReg> &regs, int coverage_bits) {
        RTLIL::SigSpec coverage_sig;
        {
            for (auto reg : regs) {
                coverage_sig.append(reg.get_sig());
            }
        }

        if (coverage_sig.size() <= coverage_bits) {
            coverage_sig.extend_u0(coverage_bits);
            assert(coverage_sig.size() == coverage_bits);
            std::string sig_name = std::string("\\coverage_" + module_name);
            RTLIL::Wire *coverage_wire =
                module->addWire(sig_name, coverage_bits);
            module->connect(coverage_wire, coverage_sig);
        } else {
            RTLIL::SigSpec coverage_sig_result(0, coverage_bits), sig;
            for (int i = 0; i < coverage_sig.size(); i++) {
                sig.append(coverage_sig[i]);
                if (sig.size() == coverage_bits ||
                    i == coverage_sig.size() - 1) {
                    coverage_sig_result =
                        module->Or(NEW_ID, coverage_sig_result, sig);
                    sig = SigSpec();
                }
            }

            std::string sig_name = std::string("\\coverage_" + module_name);
            RTLIL::Wire *coverage_wire =
                module->addWire(sig_name, coverage_bits);
            module->connect(coverage_wire, coverage_sig_result);
        }
    }

    void instrument_coverage_target(RTLIL::Module *module) {
        // Instrument target coverage signal
        RTLIL::SigSpec target_sig = get_annotated_signal(module);
        RTLIL::Wire *target_wire =
            module->addWire("\\coverage_target", target_sig.size());
        module->connect(target_wire, target_sig);
        module->fixup_ports();
    }

    virtual void execute(std::vector<std::string> args, RTLIL::Design *design) {
        log_header(design, "Executing CTRL_REG_COV pass (instrument control "
                           "register coverage).\n");
        log_push();

        int coverage_bits = 0;
        for (size_t argidx = 1; argidx < args.size(); argidx++) {
            std::string arg = args[argidx];
            if (arg == "--coverage_bits" && argidx + 1 < args.size()) {
                coverage_bits = std::stoi(args[++argidx]);
            } else {
                log_error("Failed to parse an unkown arg: %s",
                          args[argidx].c_str());
            }
        }
        assert(coverage_bits != 0);

        for (auto module : design->selected_modules()) {
            SigSet<std::pair<RTLIL::Cell *, RTLIL::IdString>> drivers;
            setup_drivers(module, drivers);

            ControlRegManager control_reg_manager;
            search_control_registers(module, drivers, control_reg_manager);
            control_reg_manager.log_stat();

            std::map<std::string, std::vector<ControlReg>> control_reg_map =
                control_reg_manager.get_control_reg_map();
            std::size_t instance_coverage_bits = coverage_bits;
            for (const auto &m : control_reg_map) {
                instrument(module, m.first, m.second, instance_coverage_bits);
            }
            instrument_coverage_target(module);
        }
    }
} DifuzzRTLPass;
