#ifndef ULI_APK_MIRRORLIST_HPP
#define ULI_APK_MIRRORLIST_HPP

#include <string>
#include <vector>
#include <utility>

namespace uli {
namespace package_mgr {
namespace apk {

class Mirrorlist {
public:
    static std::vector<std::pair<std::string, std::string>> get_alpine_mirrors() {
        return {
            {"Default", "http://dl-cdn.alpinelinux.org/alpine/"},
            {"Australia", "http://mirror.aarnet.edu.au/pub/alpine/"},
            {"Austria", "http://mirror.kumi.systems/alpine/"},
            {"Belarus", "http://mirror.datacenter.by/pub/mirrors/alpine/"},
            {"Belgium", "http://alpine.belnet.be/alpine/"},
            {"Brazil", "http://alpine.c3sl.ufpr.br/"},
            {"Bulgaria", "http://mirrors.netix.net/alpine/"},
            {"Canada", "http://alpinelinux.ca.mirror.allnetwork.com/"},
            {"Chile", "http://mirror.ufro.cl/alpine/"},
            {"China", "http://mirrors.aliyun.com/alpine/"},
            {"Costa Rica", "http://mirrors.ucr.ac.cr/alpine/"},
            {"Czech Republic", "http://ftp.sh.cvut.cz/alpine/"},
            {"Denmark", "http://mirrors.dotsrc.org/alpine/"},
            {"Ecuador", "http://mirror.cedia.org.ec/alpine/"},
            {"France", "http://ftp.halifax.rwth-aachen.de/alpine/"},
            {"Germany", "http://ftp.halifax.rwth-aachen.de/alpine/"},
            {"Hong Kong", "http://mirror.xtom.com.hk/alpine/"},
            {"Hungary", "http://quantum-mirror.hu/mirrors/pub/alpinelinux/"},
            {"Iceland", "http://mirror.system.is/alpine/"},
            {"India", "http://mirror.albony.in/alpine/"},
            {"Indonesia", "http://kartolo.sby.datautama.net.id/alpine/"},
            {"Iran", "http://mirror.iranserver.com/alpine/"},
            {"Israel", "http://mirror.isoc.org.il/pub/alpine/"},
            {"Italy", "http://alpine.mirror.garr.it/alpine/"},
            {"Japan", "http://ftp.tsukuba.wide.ad.jp/Linux/alpine/"},
            {"Kenya", "http://alpine.mirror.liquidtelecom.com/"},
            {"Mauritius", "http://mirror.marwan.ma/alpine/"},
            {"Netherlands", "http://mirror.nl.leaseweb.net/alpine/"},
            {"New Caledonia", "http://mirror.lagoon.nc/alpine/"},
            {"New Zealand", "http://mirror.fsmg.org.nz/alpine/"},
            {"Norway", "http://alpine.uib.no/"},
            {"Poland", "http://ftp.icm.edu.pl/pub/Linux/distributions/alpine/"},
            {"Portugal", "http://mirrors.ptisp.pt/alpine/"},
            {"Romania", "http://mirrors.nxthost.com/alpine/"},
            {"Russia", "http://mirror.yandex.ru/mirrors/alpine/"},
            {"Singapore", "http://mirror.sg.gs/alpine/"},
            {"South Africa", "http://alpine.is.co.za/"},
            {"South Korea", "http://mirror.kakao.com/alpine/"},
            {"Spain", "http://ftp.caliu.cat/alpine/"},
            {"Sweden", "http://ftp.acc.umu.se/mirror/alpinelinux.org/"},
            {"Switzerland", "http://mirror.init7.net/alpine/"},
            {"Taiwan", "http://ftp.yzu.edu.tw/Linux/alpine/"},
            {"Thailand", "http://mirror.psu.ac.th/alpine/"},
            {"Turkey", "http://mirror.veriteknik.net.tr/alpine/"},
            {"UK", "http://mirrors.ukfast.co.uk/sites/dl.alpinelinux.org/alpine/"},
            {"USA", "http://mirrors.edge.kernel.org/alpine/"}
        };
    }
};

} // namespace apk
} // namespace package_mgr
} // namespace uli

#endif // ULI_APK_MIRRORLIST_HPP
