#ifndef ULI_DPKG_APT_MIRRORLIST_DEBIAN_HPP
#define ULI_DPKG_APT_MIRRORLIST_DEBIAN_HPP

#include <string>
#include <vector>
#include <utility>

namespace uli {
namespace package_mgr {
namespace dpkg_apt {

class MirrorlistDebian {
public:
    static std::vector<std::pair<std::string, std::string>> get_debian_mirrors() {
        return {
            {"Default", "http://deb.debian.org/debian/"},
            {"Argentina", "http://ftp.ar.debian.org/debian/"},
            {"Australia", "http://ftp.au.debian.org/debian/"},
            {"Austria", "http://ftp.at.debian.org/debian/"},
            {"Belarus", "http://ftp.by.debian.org/debian/"},
            {"Belgium", "http://ftp.be.debian.org/debian/"},
            {"Brazil", "http://ftp.br.debian.org/debian/"},
            {"Bulgaria", "http://ftp.bg.debian.org/debian/"},
            {"Canada", "http://ftp.ca.debian.org/debian/"},
            {"Chile", "http://ftp.cl.debian.org/debian/"},
            {"China", "http://ftp.cn.debian.org/debian/"},
            {"Croatia", "http://ftp.hr.debian.org/debian/"},
            {"Czech Republic", "http://ftp.cz.debian.org/debian/"},
            {"Denmark", "http://ftp.dk.debian.org/debian/"},
            {"El Salvador", "http://ftp.sv.debian.org/debian/"},
            {"Estonia", "http://ftp.ee.debian.org/debian/"},
            {"Finland", "http://ftp.fi.debian.org/debian/"},
            {"France", "http://ftp.fr.debian.org/debian/"},
            {"Germany", "http://ftp.de.debian.org/debian/"},
            {"Greece", "http://ftp.gr.debian.org/debian/"},
            {"Hong Kong", "http://ftp.hk.debian.org/debian/"},
            {"Hungary", "http://ftp.hu.debian.org/debian/"},
            {"Iceland", "http://ftp.is.debian.org/debian/"},
            {"India", "http://ftp.in.debian.org/debian/"},
            {"Indonesia", "http://ftp.id.debian.org/debian/"},
            {"Iran", "http://ftp.ir.debian.org/debian/"},
            {"Ireland", "http://ftp.ie.debian.org/debian/"},
            {"Italy", "http://ftp.it.debian.org/debian/"},
            {"Japan", "http://ftp.jp.debian.org/debian/"},
            {"Korea", "http://ftp.kr.debian.org/debian/"},
            {"Lithuania", "http://ftp.lt.debian.org/debian/"},
            {"Mexico", "http://ftp.mx.debian.org/debian/"},
            {"Moldova", "http://ftp.md.debian.org/debian/"},
            {"Netherlands", "http://ftp.nl.debian.org/debian/"},
            {"New Caledonia", "http://ftp.nc.debian.org/debian/"},
            {"New Zealand", "http://ftp.nz.debian.org/debian/"},
            {"Norway", "http://ftp.no.debian.org/debian/"},
            {"Poland", "http://ftp.pl.debian.org/debian/"},
            {"Portugal", "http://ftp.pt.debian.org/debian/"},
            {"Romania", "http://ftp.ro.debian.org/debian/"},
            {"Russia", "http://ftp.ru.debian.org/debian/"},
            {"Singapore", "http://ftp.sg.debian.org/debian/"},
            {"Slovakia", "http://ftp.sk.debian.org/debian/"},
            {"Slovenia", "http://ftp.si.debian.org/debian/"},
            {"South Africa", "http://ftp.za.debian.org/debian/"},
            {"Spain", "http://ftp.es.debian.org/debian/"},
            {"Sweden", "http://ftp.se.debian.org/debian/"},
            {"Switzerland", "http://ftp.ch.debian.org/debian/"},
            {"Taiwan", "http://ftp.tw.debian.org/debian/"},
            {"Thailand", "http://ftp.th.debian.org/debian/"},
            {"Turkey", "http://ftp.tr.debian.org/debian/"},
            {"UK", "http://ftp.uk.debian.org/debian/"},
            {"USA", "http://ftp.us.debian.org/debian/"}
        };
    }
};

} // namespace dpkg_apt
} // namespace package_mgr
} // namespace uli

#endif // ULI_DPKG_APT_MIRRORLIST_DEBIAN_HPP
