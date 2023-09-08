// https://habr.com/ru/articles/108690/

#define MAX_CMD_LEN 1024

extern int RootExec(const char * cmd);

#if !defined(__APPLE__) && !defined(__FreeBSD__)


//https://baturin.org/docs/iproute2/
//Примечание
//Ядро Linux не хранит маршруты с недоступными следующими переходами. 
//Если ссылка выходит из строя, все маршруты, которые могли бы использовать эту ссылку, 
//навсегда удаляются из таблицы маршрутизации. 
//Возможно, вы не замечали такого поведения, потому что во многих случаях дополнительное программное обеспечение
//(например, NetworkManager или rp-pppoe) заботится о восстановлении маршрутов при изменении соединения.
//Если вы собираетесь использовать свой Linux-компьютер в качестве маршрутизатора, 
//рассмотрите возможность установки пакета протоколов маршрутизации, такого как FreeRangeRouting(https://frrouting.org/) или BIRD(https://bird.network.cz/).
//Они отслеживают состояния ссылок и восстанавливают маршруты, когда ссылка поднимается после отказа.
//Конечно, они также позволяют использовать протоколы динамической маршрутизации, такие как OSPF и BGP.

//Если вы назначите 203.0.113.25/24 для eth0, будет создан автоматически подключенный маршрут к сети 203.0.113.0/24,
//и система будет знать, что хосты из этой сети доступны напрямую.
//Когда интерфейс выходит из строя, подключенные маршруты, связанные с ним, удаляются.
//Это используется для обнаружения недоступных шлюзов, поэтому маршруты через шлюзы, ставшие недоступными, удаляются.
//Тот же механизм предотвращает создание маршрутов через недоступные шлюзы. Т.е. к примеру вы подключили по USB устройсво с драйвером g_ether http://www.linux-usb.org/gadget/,
//задали ip/mask инетрфейсу и выдернули USB - все маршрута больше нет.


//Enable IP forwarding 
//If your Linux system has a /etc/sysctl.d directory, use:

//echo 'net.ipv4.ip_forward = 1' | sudo tee -a /etc/sysctl.d/99-tailscale.conf
//echo 'net.ipv6.conf.all.forwarding = 1' | sudo tee -a /etc/sysctl.d/99-tailscale.conf
//sudo sysctl -p /etc/sysctl.d/99-tailscale.conf
//Otherwise, use:

//echo 'net.ipv4.ip_forward = 1' | sudo tee -a /etc/sysctl.conf
//echo 'net.ipv6.conf.all.forwarding = 1' | sudo tee -a /etc/sysctl.conf
//sudo sysctl -p /etc/sysctl.conf

#define SET_IP_FORWARDING "sh -c \"echo %u > /proc/sys/net/ipv4/ip_forward\""
#define SET_IP6_FORWARDING "sh -c \"echo %u > /proc/sys/net/ipv6/conf/all/forwarding\""
//[ "$(echo `sudo iptables -S FORWARD` | head -n 1)" = "-P FORWARD ACCEPT" ] || sudo iptables -P FORWARD ACCEPT

//TODO: Сброс таблицы (ARP и NDP) ip neighbor flush dev ${interface name}
//TODO: Добавляем запись в (ARP и NDP) таблицу ip neighbor add ${network address} lladdr ${link layer address} dev ${interface name}
//Примеры:ip neighbor add 192.0.2.1 lladdr 22:ce:e0:99:63:6f dev eth0
//TODO: Удаляем запись в (ARP и NDP) таблицы ip neighbor delete ${network address} lladdr ${link layer address} dev ${interface name}
//Примеры:ip neighbor delete 192.0.2.1 lladdr 22:ce:e0:99:63:6f dev eth0

//Просмотр маршрутов
//TODO: Просмотр маршрутов к сети и всем ее подсетям ip route show to root ${address}/${mask}
//Например, если вы используете в своей сети подсеть 192.168.0.0/24 и она разбита на 192.168.0.0/25 и 192.168.0.128/25, вы можете увидеть все эти маршруты с расширением ip route show to root 192.168.0.0/24.
//TODO: Просмотр маршрутов к сети(адресу) по всем записям маршрутов ip route show to match ${address}/${mask}
//Маршрутизаторы предпочитают более конкретные маршруты менее конкретным, поэтому это часто полезно для отладки в ситуациях,
//когда трафик в определенную подсеть направляется неправильным путем, поскольку маршрут к ней отсутствует, 
//но существуют маршруты к более крупным подсетям.
//TODO: Просмотр маршрутов к конкретной подсети ip route show to exact ${address}/${mask}
//Если вы хотите видеть маршруты к 192.168.0.0/24, но не к, скажем, 192.168.0.0/25 и 192.168.0.0/16,
//вы можете использовать ip route show to exact 192.168.0.0/24.
//TODO: Просмотр только маршрута, фактически используемого ядром ip route get ${address}/${mask}
//Пример: ip route get 192.168.0.0/24.
//TODO: Просмотр кэша маршрутов (только ядра до версии 3.6) ip route show cached
//TODO: Добавляем маршрут через шлюз ip route add ${address}/${mask} via ${next hop}
//Примеры:
//ip route add 192.0.2.128/25 via 192.0.2.1
//ip route add 2001:db8:1::/48 via 2001:db8:1::1
//TODO: Добавляем маршрут через интерфейс ip route add ${address}/${mask} dev ${interface name}
//Пример:
//ip route add 192.0.2.0/25 dev ppp0
//Маршруты интерфейса обычно используются с интерфейсами точка-точка, такими как туннели PPP, где адрес следующего перехода не требуется.
//TODO:  Добавляем маршрут без проверки доступности шлюза ip route add ${address}/${mask} dev ${interface name} onlink
//Пример:
//ip route add 192.0.2.128/25 via 203.0.113.1 dev eth0 onlink
//Ключевое onlink слово отключает проверки целостности шлюза ядра и позволяет добавлять маршруты через шлюзы,
//которые выглядят недоступными. Полезно в туннельных конфигурациях и сетях контейнеров/виртуализаций с несколькими 
//сетями на одном и том же канале с использованием одного шлюза.
// Ключевое onlink слово отключает проверки целостности шлюза ядра и позволяет добавлять маршруты через шлюзы, которые выглядят недоступными.
// Полезно в туннельных конфигурациях и сетях контейнеров/виртуализаций с несколькими сетями на одном и том же канале с использованием одного шлюза.

// Для модификации маршрутов есть ip route changeи ip route replace команды.
// Разница между ними в том, что changeкоманда выдаст ошибку, если вы попытаетесь изменить несуществующий маршрут.
// Команда replace создаст маршрут, если он еще не существует.
//$ ip route show
//192.168.1.0/24 dev eth0  proto kernel  scope link  src 192.168.1.100  metric 1 
//169.254.0.0/16 dev eth0  scope link  metric 1000 
//default via 192.168.1.1 dev eth0  proto static 
//Step 2: Change the default settings.
//Paste the current settings for default and add initcwnd 10 to it.
//$sudo ip route change default via 192.168.1.1 dev eth0  proto static initcwnd 10

// Маршрут по умолчанию
//Существует ярлык для создания маршрутов по умолчанию.
//ip route add default via ${address}/${mask}
//ip route add default dev ${interface name}
//Они эквивалентны:
//ip route add 0.0.0.0/0 via ${address}/${mask}
//ip route add 0.0.0.0/0 dev ${interface name}
//Для маршрутов IPv6 default эквивалентен ::/0.
//ip -6 route add default via 2001:db8::1

// Маршруты черной дыры
// ip route add blackhole ${address}/${mask}
// Примеры: ip route add blackhole 192.0.2.1/32.

//ip route add unreachable ${address}/${mask}
//ip route add prohibit ${address}/${mask}
//ip route add throw ${address}/${mask}
//Эти маршруты заставляют систему отбрасывать пакеты и отвечать отправителю сообщением об ошибке ICMP.
//unreachable: отправляет ICMP «хост недоступен».
//prohibit: отправляет ICMP «административно запрещено».
//throw: Отправляет «сеть недоступна».
//Маршруты Throw могут использоваться для реализации маршрутизации на основе политик. В нестандартных таблицах они
//останавливают процесс поиска, но не отправляют сообщения об ошибках ICMP.

// Маршруты с другой метрикой
//ip route add ${address}/${mask} via ${gateway} metric ${number}
//Примеры:
//ip route add 192.168.2.0/24 via 10.0.1.1 metric 5
//ip route add 192.168.2.0 dev ppp0 metric 10
//Если есть несколько маршрутов к одной и той же сети с разным значением метрики, ядро ​​отдаепредпочтение маршруту с наименьшей метрикой .
// Важной частью этой концепции является то, что когда интерфейс выходит из строя, маршруты, которые в результате этого события станут бесполезными,
// исчезают из таблицы маршрутизации (см. раздел о подключенных маршрутах ), и система возвращается к маршрутам с более высокими значениями метрик.


//Многопутевая маршрутизация
//ip route add ${addresss}/${mask} nexthop via ${gateway 1} weight ${number} nexthop via ${gateway 2} weight ${number}
//Многопутевые маршруты заставляют систему балансировать пакеты по нескольким каналам в соответствии с их весом (более высокий вес предпочтителен,
//поэтому шлюз/интерфейс с весом 2 получит примерно в два раза больше трафика, чем другой шлюз с весом 1).
// У вас может быть столько шлюзов, сколько вы хотите, и вы можете смешивать маршруты шлюзов и интерфейсов:
//ip route add default nexthop via 192.168.1.1 weight 1 nexthop dev ppp0 weight 10


//Когда вы добавляете адрес на свой компьютер, в любой интерфейс, он создает специальный маршрут в таблице local(проверьте свой sudo ip route show table local).
//Будет несколько local маршрутов. Любая связь с локальными маршрутами предназначена для пропуска нескольких уровней сетевого стека для повышения эффективности.
//Ваши пакеты должны появиться на lo интерфейсе, поэтому, чтобы увидеть их, вам нужно запустить sudo tshark -i lo

#else

// Добавить/удалить маршрут временный
// sudo route -n add -net 192.168.2.0/24 192.168.1.1
// sudo route add -host 54.81.143.201 -interface en0
// sudo route add -host 54.81.143.201 -link 14:10:9f:e7:fd:0a -static
// sudo route add -net 10.13.0.0 netmask 255.255.0.0 
// sudo route -n delete 192.168.2.0/24
// Добавить маршрут постоянный:
// 1) шаг вывести список networksetup -listnetworkserviceorder или networksetup -listallnetworkservices
// (1) USB 10/100/1000 LAN
// (Hardware Port: USB 10/100/1000 LAN, Device: en7)
// (2) Wi-Fi
// (Hardware Port: Wi-Fi, Device: en0)
// (3) Bluetooth PAN
// (Hardware Port: Bluetooth PAN, Device: en6)
// (4) Thunderbolt Bridge
// (Hardware Port: Thunderbolt Bridge, Device: bridge0)
// список маршрутов networksetup -getadditionalroutes "USB 10/100/1000 LAN"
// 2) шаг sudo networksetup -setadditionalroutes "USB 10/100/1000 LAN" 192.168.2.0 255.255.255.0 192.168.1.30
// 3) шаг удалить sudo networksetup -setadditionalroutes "USB 10/100/1000 LAN"

#define SET_IP_FORWARDING "sysctl -w net.inet.ip.forwarding=%d"
#define SET_IP6_FORWARDING "sysctl -w net.inet6.ip6.forwarding=%d"

#endif
