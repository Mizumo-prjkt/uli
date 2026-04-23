// testing.cpp - Test entry point for HarukaInstaller TUI
// Compile with -DTESTUI to enable.
// This uses simulation_test.hpp for mock data so the UI can be tested
// without real hardware detection.

#ifdef TESTUI

#include "simulation_test.hpp"
#include "../ncurseslib.hpp"
#include "../mainmenu/mainmenu.hpp"
#include "../popups.hpp"
#include "../configurations/datastore.hpp"

// Page includes
#include "../mainmenu/pages/language_page.hpp"
#include "../mainmenu/pages/mirror_page.hpp"
#include "../mainmenu/pages/keyboard_locale_page.hpp"
#include "../mainmenu/pages/disk_page.hpp"
#include "../mainmenu/pages/zram_page.hpp"
#include "../mainmenu/pages/graphics_page.hpp"
#include "../mainmenu/pages/profile_page.hpp"
#include "../mainmenu/pages/network_page.hpp"
#include "../mainmenu/pages/accounts_page.hpp"
#include "../mainmenu/pages/kernels_page.hpp"
#include "../mainmenu/pages/timedate_page.hpp"
#include "../mainmenu/pages/bootloader_page.hpp"
#include "../mainmenu/pages/stub_page.hpp"

int main() {
    // ── Load simulation data ────────────────────────────────────────────
    auto sim_langs     = SimData::get_languages();
    auto sim_mirrors   = SimData::get_mirrors();
    auto sim_kblayouts = SimData::get_keyboard_layouts();
    auto sim_locales   = SimData::get_locales();
    auto sim_disks     = SimData::get_disks();
    auto sim_networks  = SimData::get_network_interfaces();
    auto sim_wifi      = SimData::get_wifi_networks();
    auto sim_kernels   = SimData::get_kernels();
    auto sim_tz        = SimData::get_timezones();
    auto sim_gpus      = SimData::get_gpus();
    auto sim_profiles  = SimData::get_profiles();

    // ── Populate DataStore ──────────────────────────────────────────────
    auto& ds = DataStore::instance();
    
    for (auto& l : sim_langs)
        ds.languages.push_back({l.code, l.name});
        
    for (auto& m : sim_mirrors)
        ds.mirrors.push_back({m.url, m.country, m.enabled});
        
    for (auto& k : sim_kblayouts)
        ds.keyboard_layouts.push_back({k.code, k.name});
        
    ds.locales = sim_locales;
        
    for (auto& d : sim_disks) {
        DiskInfo di;
        di.device = d.device;
        di.model = d.model;
        di.size_mb = d.size_mb;
        di.table_type = d.table_type;
        for (auto& p : d.partitions)
            di.partitions.push_back({p.device, p.mount_point, p.size_mb, p.filesystem, p.flags});
        ds.disks.push_back(di);
    }
    
    for (auto& k : sim_kernels)
        ds.kernels.push_back({k.package, k.description, false});
    if (!ds.kernels.empty()) ds.kernels[0].selected = true;

    std::vector<NetIfaceInfo> net_items;
    for (auto& n : sim_networks)
        net_items.push_back({n.name, n.type, n.connected, n.ip_address,
                             n.mac_address, n.gateway, n.dns, n.use_dhcp});

    std::vector<WifiNetwork> wifi_items;
    for (auto& w : sim_wifi)
        wifi_items.push_back({w.ssid, w.signal, w.secured, w.connected});

    std::vector<TZRegion> tz_items;
    for (auto& t : sim_tz)
        tz_items.push_back({t.region, t.cities});

    std::vector<GPUInfo> gpu_items;
    for (auto& g : sim_gpus)
        gpu_items.push_back({g.name, g.vendor, g.driver_rec});

    for (auto& p : sim_profiles)
        ds.profiles.push_back({p.name, p.description, p.default_de});

    // ── Initialize ncurses ──────────────────────────────────────────────
    NcursesLib ncurses;
    ncurses.init_ncurses();

    // ── Build main menu ─────────────────────────────────────────────────
    MainMenu menu;

    menu.add_page("Installer Language",          new LanguagePage());
    menu.add_page("Mirror Configuration",        new MirrorPage());
    menu.add_page("Keyboard and Locale",         new KeyboardLocalePage());
    menu.add_page("Storage Device Configuration", new DiskPage());
    menu.add_page("ZRAM and ZSWAP",              new ZramPage());
    menu.add_page("Graphics Hardware",           new GraphicsPage(gpu_items));
    menu.add_page("Profile",                     new ProfilePage());
    menu.add_page("Network",                     new NetworkPage(net_items, wifi_items));
    menu.add_page("Root and User Accounts",      new AccountsPage());
    menu.add_page("Kernels",                     new KernelsPage());
    menu.add_page("Bootloader",                  new BootloaderPage());
    menu.add_page("Time and Date",               new TimeDatePage(tz_items));
    menu.add_page("Audio",                       new StubPage("Audio"));
    menu.add_page("Services",                    new StubPage("Services"));
    menu.add_page("Additional Packages",         new StubPage("Additional Packages"));

    menu.add_separator();
    menu.add_action("Save Config");
    menu.add_action("INSTALL!");
    menu.add_action("Abort");

    // ── Run ─────────────────────────────────────────────────────────────
    menu.run();

    // ── Cleanup ─────────────────────────────────────────────────────────
    ncurses.end_ncurses();
    return 0;
}

#endif // TESTUI
