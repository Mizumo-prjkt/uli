#ifndef ULI_DBUS_DBUSPOINT_HPP
#define ULI_DBUS_DBUSPOINT_HPP

/*
 * ============================================================================
 *  ULI D-Bus Abstraction Layer  (dbuspoint.hpp)
 * ============================================================================
 *
 *  PURPOSE:
 *    Provides a thin C++ wrapper around the low-level libdbus-1 C API so that
 *    other modules in the installer (network backends, etc.) can communicate
 *    with system daemons over the D-Bus System Bus without touching raw C
 *    pointers directly.
 *
 *  HOW D-BUS COMMUNICATION WORKS (for future maintainers):
 *  -------------------------------------------------------
 *
 *  D-Bus is an IPC (Inter-Process Communication) system used on Linux.
 *  It allows processes to talk to each other via "messages" sent over a
 *  shared message bus. There are two buses:
 *
 *    - SESSION bus : per-user desktop services (not used here)
 *    - SYSTEM  bus : system-wide daemons like NetworkManager, iwd,
 *                    wpa_supplicant, systemd, etc.  <-- THIS IS WHAT WE USE
 *
 *  Every D-Bus interaction follows this addressing model:
 *
 *    ┌──────────────────────────────────────────────────────────────────┐
 *    │  SERVICE (bus name)    e.g. "net.connman.iwd"                   │
 *    │    └─ OBJECT PATH      e.g. "/net/connman/iwd/0/4"             │
 *    │         └─ INTERFACE   e.g. "net.connman.iwd.Station"          │
 *    │              └─ METHOD e.g. "Scan"                             │
 *    └──────────────────────────────────────────────────────────────────┘
 *
 *  To call a D-Bus method, we construct a DBusMessage with:
 *    - destination  = the service bus name
 *    - path         = the object path
 *    - interface    = the interface name
 *    - method       = the method name
 *  Then we send it on the bus connection and read the reply.
 *
 *  READING PROPERTIES:
 *    D-Bus properties are read through a standard interface:
 *      org.freedesktop.DBus.Properties  →  Get(interface, property_name)
 *    This returns a VARIANT containing the actual value.
 *
 *  INTROSPECTION:
 *    Every D-Bus object supports org.freedesktop.DBus.Introspectable
 *    with a method Introspect() that returns an XML description of the
 *    object's interfaces, methods, signals, and child object paths.
 *    We use this to enumerate child objects (e.g., discover all IWD
 *    device/station/network objects under /net/connman/iwd/).
 *
 *  SERVICE ACTIVATION CHECK:
 *    We use org.freedesktop.DBus → NameHasOwner(name) to check whether
 *    a given daemon (like "net.connman.iwd") is currently running on
 *    the system bus.
 *
 *  SERVICES WE TALK TO:
 *  --------------------
 *
 *  1) IWD (iNet Wireless Daemon):
 *     Bus name:   net.connman.iwd
 *     Station:    net.connman.iwd.Station   (Scan, GetOrderedNetworks)
 *     Network:    net.connman.iwd.Network   (Connect)
 *     Device:     net.connman.iwd.Device    (Name property = interface name)
 *
 *  2) wpa_supplicant:
 *     Bus name:   fi.w1.wpa_supplicant1
 *     Root:       fi.w1.wpa_supplicant1     (GetInterface, CreateInterface)
 *     Interface:  fi.w1.wpa_supplicant1.Interface (Scan, AddNetwork, SelectNetwork)
 *     BSS:        fi.w1.wpa_supplicant1.BSS (SSID property)
 *
 *  FALLBACK STRATEGY:
 *    If a D-Bus call fails at runtime (daemon not running, permission denied,
 *    etc.), the backends (iwd_backend.hpp, wpa_backend.hpp) fall back to
 *    popen() CLI wrappers (iwctl, wpa_cli) automatically. This ensures the
 *    installer works even on minimal environments where dbus access is
 *    restricted.
 *
 *  BUILD DEPENDENCY:
 *    This file requires libdbus-1 headers and library.
 *    CMakeLists.txt discovers it via:
 *      pkg_check_modules(DBUS REQUIRED dbus-1)
 *    and links it to the uli_installer target.
 *
 * ============================================================================
 */

#include <dbus/dbus.h>
#include <string>
#include <vector>
#include <iostream>

namespace uli {
namespace dbus {

class DBusPoint {
public:

    /*
     * get_connection()
     * ----------------
     * Opens a shared connection to the SYSTEM D-Bus.
     *
     * How it works:
     *   1. dbus_bus_get(DBUS_BUS_SYSTEM, &err) asks libdbus to connect
     *      to the well-known system bus socket (usually /var/run/dbus/system_bus_socket).
     *   2. The connection is "shared" — multiple calls return the same pointer.
     *   3. We do NOT call dbus_connection_unref() because shared connections
     *      are managed by libdbus internally.
     *
     * Returns: A valid DBusConnection*, or nullptr on failure.
     */
    static DBusConnection* get_connection() {
        DBusError err;
        dbus_error_init(&err);

        DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

        if (dbus_error_is_set(&err)) {
            std::cerr << "[DBusPoint] Connection error: " << err.message << std::endl;
            dbus_error_free(&err);
            return nullptr;
        }

        return conn;
    }

    /*
     * is_service_active(service_name)
     * --------------------------------
     * Checks whether a given D-Bus service is currently running.
     *
     * How it works:
     *   We call the D-Bus daemon itself (org.freedesktop.DBus) and invoke
     *   the NameHasOwner method, passing the service name as a string argument.
     *
     *   D-Bus call anatomy:
     *     destination:  "org.freedesktop.DBus"      (the bus daemon itself)
     *     path:         "/org/freedesktop/DBus"
     *     interface:    "org.freedesktop.DBus"
     *     method:       "NameHasOwner"
     *     argument:     STRING service_name
     *     returns:      BOOLEAN (true if the service is active)
     *
     * Example:
     *   is_service_active("net.connman.iwd")
     *     → sends: NameHasOwner("net.connman.iwd")
     *     → returns true if iwd daemon is running
     */
    static bool is_service_active(const std::string& service_name) {
        DBusConnection* conn = get_connection();
        if (!conn) return false;

        DBusMessage* msg = dbus_message_new_method_call(
            "org.freedesktop.DBus",       // destination: the bus daemon
            "/org/freedesktop/DBus",       // object path
            "org.freedesktop.DBus",        // interface
            "NameHasOwner"                 // method
        );
        if (!msg) return false;

        // Append the service name we're checking as a string argument
        const char* name_cstr = service_name.c_str();
        dbus_message_append_args(msg,
            DBUS_TYPE_STRING, &name_cstr,
            DBUS_TYPE_INVALID);

        DBusError err;
        dbus_error_init(&err);

        // Send the message and block for the reply (timeout: 2 seconds)
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(
            conn, msg, 2000, &err);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&err)) {
            dbus_error_free(&err);
            return false;
        }
        if (!reply) return false;

        // Extract the boolean result from the reply
        dbus_bool_t has_owner = FALSE;
        dbus_message_get_args(reply, &err,
            DBUS_TYPE_BOOLEAN, &has_owner,
            DBUS_TYPE_INVALID);
        dbus_message_unref(reply);

        if (dbus_error_is_set(&err)) {
            dbus_error_free(&err);
            return false;
        }

        return has_owner == TRUE;
    }

    /*
     * call_method(service, path, interface, method)
     * -----------------------------------------------
     * Sends a D-Bus method call with no arguments and returns the raw reply.
     *
     * How it works:
     *   1. Constructs a DBusMessage of type METHOD_CALL.
     *   2. Sends it synchronously and blocks for the reply.
     *   3. Returns the reply DBusMessage* — caller is responsible for
     *      extracting data and calling dbus_message_unref().
     *
     * This is the lowest-level call helper. For more complex calls
     * (with arguments), the backends construct messages directly.
     *
     * Returns: DBusMessage* reply, or nullptr on error.
     *          The CALLER must call dbus_message_unref() on the result.
     */
    static DBusMessage* call_method(
        const std::string& service,
        const std::string& path,
        const std::string& interface,
        const std::string& method,
        int timeout_ms = 5000
    ) {
        DBusConnection* conn = get_connection();
        if (!conn) return nullptr;

        DBusMessage* msg = dbus_message_new_method_call(
            service.c_str(),
            path.c_str(),
            interface.c_str(),
            method.c_str()
        );
        if (!msg) return nullptr;

        DBusError err;
        dbus_error_init(&err);

        DBusMessage* reply = dbus_connection_send_with_reply_and_block(
            conn, msg, timeout_ms, &err);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&err)) {
            std::cerr << "[DBusPoint] Method call failed (" 
                      << interface << "." << method << "): " 
                      << err.message << std::endl;
            dbus_error_free(&err);
            return nullptr;
        }

        return reply;
    }

    /*
     * get_string_property(service, path, interface, property_name)
     * ---------------------------------------------------------------
     * Reads a single STRING property from a D-Bus object.
     *
     * How it works:
     *   Properties in D-Bus are accessed through a standard proxy interface:
     *     org.freedesktop.DBus.Properties → Get(interface_name, property_name)
     *
     *   The reply contains a VARIANT wrapping the actual value.
     *   We open the variant iterator and extract the string.
     *
     *   D-Bus call anatomy:
     *     destination:  service
     *     path:         object path
     *     interface:    "org.freedesktop.DBus.Properties"
     *     method:       "Get"
     *     arg[0]:       STRING interface_name  (which interface owns the property)
     *     arg[1]:       STRING property_name
     *     returns:      VARIANT → STRING
     */
    static std::string get_string_property(
        const std::string& service,
        const std::string& path,
        const std::string& interface,
        const std::string& property_name
    ) {
        DBusConnection* conn = get_connection();
        if (!conn) return "";

        DBusMessage* msg = dbus_message_new_method_call(
            service.c_str(),
            path.c_str(),
            "org.freedesktop.DBus.Properties",  // Standard properties interface
            "Get"
        );
        if (!msg) return "";

        const char* iface_cstr = interface.c_str();
        const char* prop_cstr = property_name.c_str();
        dbus_message_append_args(msg,
            DBUS_TYPE_STRING, &iface_cstr,
            DBUS_TYPE_STRING, &prop_cstr,
            DBUS_TYPE_INVALID);

        DBusError err;
        dbus_error_init(&err);

        DBusMessage* reply = dbus_connection_send_with_reply_and_block(
            conn, msg, 3000, &err);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&err)) {
            dbus_error_free(&err);
            return "";
        }
        if (!reply) return "";

        /*
         * The reply message contains a VARIANT as the top-level argument.
         * We must:
         *   1. Get an iterator on the reply message
         *   2. Open the variant sub-iterator
         *   3. Read the string from inside the variant
         */
        DBusMessageIter iter;
        dbus_message_iter_init(reply, &iter);

        if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
            dbus_message_unref(reply);
            return "";
        }

        DBusMessageIter variant_iter;
        dbus_message_iter_recurse(&iter, &variant_iter);

        if (dbus_message_iter_get_arg_type(&variant_iter) != DBUS_TYPE_STRING) {
            dbus_message_unref(reply);
            return "";
        }

        const char* val = nullptr;
        dbus_message_iter_get_basic(&variant_iter, &val);
        std::string result = val ? val : "";

        dbus_message_unref(reply);
        return result;
    }

    /*
     * introspect_children(service, path)
     * ------------------------------------
     * Lists all immediate child object paths under a given D-Bus path.
     *
     * How it works:
     *   Every D-Bus object supports org.freedesktop.DBus.Introspectable
     *   with a method Introspect() that returns an XML string describing
     *   the object tree. We parse child <node name="..."/> entries from
     *   the XML to discover sub-objects.
     *
     *   Example XML fragment returned by Introspect() on "/net/connman/iwd":
     *     <node>
     *       <node name="0"/>
     *       <node name="1"/>
     *     </node>
     *
     *   From this we extract ["0", "1"] and prepend the parent path to get:
     *     ["/net/connman/iwd/0", "/net/connman/iwd/1"]
     *
     *   This is used to walk the IWD object tree and find Device/Station
     *   objects dynamically without hardcoding paths.
     */
    static std::vector<std::string> introspect_children(
        const std::string& service,
        const std::string& path
    ) {
        std::vector<std::string> children;

        DBusMessage* reply = call_method(
            service, path,
            "org.freedesktop.DBus.Introspectable",
            "Introspect"
        );
        if (!reply) return children;

        const char* xml = nullptr;
        DBusError err;
        dbus_error_init(&err);
        dbus_message_get_args(reply, &err,
            DBUS_TYPE_STRING, &xml,
            DBUS_TYPE_INVALID);

        if (dbus_error_is_set(&err) || !xml) {
            dbus_error_free(&err);
            dbus_message_unref(reply);
            return children;
        }

        /*
         * Simple XML parsing for <node name="X"/> entries.
         * We don't pull in a full XML parser — just find the pattern.
         */
        std::string xml_str(xml);
        std::string search_token = "<node name=\"";
        size_t pos = 0;
        while ((pos = xml_str.find(search_token, pos)) != std::string::npos) {
            pos += search_token.length();
            size_t end = xml_str.find("\"", pos);
            if (end != std::string::npos) {
                std::string child_name = xml_str.substr(pos, end - pos);
                // Build full child path
                std::string child_path = path;
                if (child_path.back() != '/') child_path += "/";
                child_path += child_name;
                children.push_back(child_path);
                pos = end;
            }
        }

        dbus_message_unref(reply);
        return children;
    }

    /*
     * has_interface(service, path, interface_name)
     * -----------------------------------------------
     * Checks if a D-Bus object at the given path implements a specific interface.
     *
     * How it works:
     *   Calls Introspect() on the object and searches the returned XML for
     *   an <interface name="..."/> entry matching the requested interface.
     *
     *   This is used to filter objects — e.g., to find which child objects
     *   under /net/connman/iwd are Station objects vs Device objects.
     */
    static bool has_interface(
        const std::string& service,
        const std::string& path,
        const std::string& interface_name
    ) {
        DBusMessage* reply = call_method(
            service, path,
            "org.freedesktop.DBus.Introspectable",
            "Introspect"
        );
        if (!reply) return false;

        const char* xml = nullptr;
        DBusError err;
        dbus_error_init(&err);
        dbus_message_get_args(reply, &err,
            DBUS_TYPE_STRING, &xml,
            DBUS_TYPE_INVALID);

        bool found = false;
        if (!dbus_error_is_set(&err) && xml) {
            std::string xml_str(xml);
            std::string pattern = "<interface name=\"" + interface_name + "\"";
            found = (xml_str.find(pattern) != std::string::npos);
        }

        if (dbus_error_is_set(&err)) dbus_error_free(&err);
        dbus_message_unref(reply);
        return found;
    }

    /*
     * find_objects_with_interface(service, root_path, interface_name)
     * -----------------------------------------------------------------
     * Recursively walks the D-Bus object tree starting from root_path and
     * returns all object paths that implement the given interface.
     *
     * How it works:
     *   1. Introspect the root_path to get child nodes
     *   2. For each child, check if it has the target interface
     *   3. Recurse into children to find deeper matches
     *
     * Example:
     *   find_objects_with_interface("net.connman.iwd", "/net/connman/iwd",
     *                              "net.connman.iwd.Station")
     *   might return: ["/net/connman/iwd/0/4"]
     *
     * This is how we discover which IWD object path corresponds to
     * a wireless station (so we can call Scan on it).
     */
    static std::vector<std::string> find_objects_with_interface(
        const std::string& service,
        const std::string& root_path,
        const std::string& interface_name
    ) {
        std::vector<std::string> result;

        // Check if root itself has the interface
        if (has_interface(service, root_path, interface_name)) {
            result.push_back(root_path);
        }

        // Recurse into children
        auto children = introspect_children(service, root_path);
        for (const auto& child : children) {
            auto sub = find_objects_with_interface(service, child, interface_name);
            result.insert(result.end(), sub.begin(), sub.end());
        }

        return result;
    }

    /*
     * call_method_void(service, path, interface, method)
     * ----------------------------------------------------
     * Sends a D-Bus method call that takes no arguments and returns no
     * meaningful result. Used for fire-and-forget calls like Station.Scan().
     *
     * Returns true if the call succeeded (no D-Bus error), false otherwise.
     */
    static bool call_method_void(
        const std::string& service,
        const std::string& path,
        const std::string& interface,
        const std::string& method,
        int timeout_ms = 10000
    ) {
        DBusMessage* reply = call_method(service, path, interface, method, timeout_ms);
        if (!reply) return false;
        dbus_message_unref(reply);
        return true;
    }
};

} // namespace dbus
} // namespace uli

#endif // ULI_DBUS_DBUSPOINT_HPP
