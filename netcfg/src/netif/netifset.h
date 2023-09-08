#define MAX_CMD_LEN 512

extern int RootExec(const char * cmd);

//TODO: tcpdump add parametrs #define TCPDUMP_CMD   "tcpdump --interface=%S --buffer-size=8192 -w %S -C 10000000 -Z root -W 10 &"

#define TCPDUMP_CMD     "tcpdump --interface=%S --buffer-size=8192 --monitor-mode -w %S || tcpdump --interface=%S --buffer-size=8192 -w %S &"
#define TCPDUMPKILL_CMD "pkill -f '^tcpdump --interface=%S'"

#if !defined(__APPLE__) && !defined(__FreeBSD__)

// TODO: Установить удобочитаемое описание ip link set dev ${interface name} alias "${description}"
// Примеры: ip link set dev eth0 alias "LAN interface".
//TODO: #define SETTRAILERSON_CMD "ip link set dev %S trailers on"
//TODO: #define SETTRAILERSOFF_CMD "ip link set dev %S trailers off"

//TODO: #define SETDYNAMICON_CMD "ip link set dev %S dynamic on"
//TODO: #define SETDYNAMICOFF_CMD "ip link set dev %S dynamic off"

//TODO: #define SETCARRIERON_CMD "ip link set dev %S carrier on"
//TODO: #define SETCARRIEROFF_CMD "ip link set dev %S carrier off"

//TODO: Создание интерфейса IEEE 802.1q VLAN ip link add name ${VLAN interface name} link ${parent interface name} type vlan id ${tag}
//Примеры:ip link add name eth0.110 link eth0 type vlan id 110

//TODO: Создать интерфейс 802.1ad QinQ (стекирование VLAN) способ передачи тегированного трафика VLAN через другую VLAN
//Сервисный тег — это тег VLAN, который провайдер использует для передачи клиентского трафика через свою сеть. Тег клиента — это тег, установленный клиентом.
// ip link add name ${service interface} link ${physical interface} type vlan proto 802.1ad id ${service tag}
// ip link add name ${client interface} link ${service interface} type vlan proto 802.1q id ${client tag}
//Пример:
//ip link add name eth0.100 link eth0 type vlan proto 802.1ad id 101 # Create a service tag interface
//ip link add name eth0.100.200 link eth0.100 type vlan proto 802.1q id 201 # Create a client tag interface
//Обычный вариант использования такой: предположим, вы поставщик услуг и у вас есть клиент, который хочет
//   использовать вашу сетевую инфраструктуру для соединения частей своей сети друг с другом. Они используют
//   несколько VLAN в своей сети, поэтому обычная арендованная VLAN не вариант. С QinQ вы можете добавить второй
//   тег к трафику клиента, когда он входит в вашу сеть, и удалить этот тег, когда он выходит, чтобы не было конфликтов,
//   и вам не нужно тратить номера VLAN.
//Обратите внимание, что MTU канала для клиентского интерфейса VLAN не настраивается автоматически; 
//   вам нужно позаботиться об этом самостоятельно и либо уменьшить MTU клиентского интерфейса как минимум на 4 байта,
//   либо соответственно увеличить родительский MTU.
//QinQ, соответствующий стандартам, доступен, начиная с Linux 3.10.

//TODO: Создать виртуальный интерфейс (MACVLAN) ip link add name ${macvlan interface name} link ${parent interface} type macvlan
//Примеры:ip link add name peth0 link eth0 type macvlan
//Интерфейсы MACVLAN похожи на «вторичные MAC-адреса» на одном интерфейсе. 
//   С точки зрения пользователя они выглядят как обычные интерфейсы Ethernet и обрабатывают весь трафик для своего MAC-адреса.
//Это можно использовать для служб и приложений, которые плохо обрабатывают вторичные IP-адреса.

//TODO: Создать фиктивный интерфейс ip link add name ${dummy interface name} type dummy
// Примеры:ip link add name dummy0 type dummy
//В Linux по каким-то странным историческим причинам есть только один loopback интерфейс. Интерфейсы-пустышки dummy похожи на loopback, но их может быть много.
//Их можно использовать для связи внутри одного хоста. loopback или фиктивный dummy интерфейс также являются хорошим местом для назначения адреса 
//управления на маршрутизаторе с несколькими физическими интерфейсами.

//TODO: Создать мостовой интерфейс ip link add name ${bridge name} type bridge
//Примеры:ip link add name br0 type bridge
//Мостовые интерфейсы — это виртуальные коммутаторы Ethernet. Вы можете использовать их, чтобы превратить машину Linux в медленный коммутатор L2
//   или обеспечить связь между виртуальными машинами на хосте гипервизора. Обратите внимание, что превращение Linux-системы в физический коммутатор
//   не является совершенно абсурдной идеей, поскольку, в отличие от глупых аппаратных коммутаторов, он может работать как прозрачный брандмауэр.
//Вы можете назначить мосту IP-адрес, и он будет виден со всех портов моста.
//Если создание моста не удается, проверьте, bridgeзагружен ли модуль.
//TODO: Добавляем мостовой порт ip link set dev ${interface name} master ${bridge name}
//Примеры:ip link set dev eth0 master br0
//Интерфейс, который вы добавляете к мосту, становится портом виртуального коммутатора. 
//    Он работает только на канальном уровне и прекращает работу на сетевом уровне.
//TODO: Удалить мостовой порт ip link set dev ${interface name} nomaster
//Примеры:ip link set dev eth0 nomaster

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specific cases
//TODO: Создать связующий интерфейс ip link add name ${name} type bond
//Примеры:ip link add name bond1 type bond
//Примечание. Этого недостаточно для какой-либо осмысленной настройки связывания (агрегации ссылок). 
//Необходимо настроить параметры связывания в соответствии с вашей ситуацией.
// Элементы связующего интерфейса добавляются и удаляются точно так же, как мостовые порты, с помощью команд master и nomaster.

//TODO: Создать intermediate functional block interface ip link add ${interface name} type ifb
//Пример:ip link add ifb10 type ifb
//Промежуточные функциональные блочные устройства используются для перенаправления и зеркалирования трафика совместно с tc.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: Создаем пару виртуальных сетевых устройств  ip link add name ${first device name} type veth peer name ${second device name}
//Примеры:ip link add name veth-host type veth peer name veth-guest
//Устройства виртуального Ethernet (veth) всегда идут парами и работают как двунаправленный канал: все, что входит в одно из них, 
//    выходит из другого. Они используются в сочетании с функциями разделения системы, такими как сетевые пространства имен 
//    и контейнеры (OpenVZ или LXC), для соединения одного с другим.
//Примечание. В зависимости от версии ядра эти устройства могут быть созданы как в выключенном, так и в рабочем состоянии.
//В последних версиях (проверено в 5.10) они не работают. Лучше предположить, что они не работают, и всегда использовать ip link set ${intf} upна них.

//TODO: Управление группой link
//Вы можете добавлять сетевые интерфейсы в пронумерованную группу и выполнять операции сразу на всех интерфейсах из этой группы.
//Ссылки, не отнесенные ни к одной группе, относятся к группе 0 («по умолчанию»).
//TODO: Добавляем интерфейс в группу ip link set dev ${interface name} group ${group number}
//Примеры:
//ip link set dev eth0 group 42
//ip link set dev eth1 group 42
//TODO: # Удалить интерфейс из группы Это можно сделать, назначив его группе по умолчанию. ip link set dev ${interface name} group 0 или ip link set dev ${interface} group default
//Примеры:ip link set dev tun10 group 0
//TODO: Присвоить символическое имя группе ${number} ${name} в /etc/iproute2/group файле
//Имена групп настраиваются в /etc/iproute2/group файле. Символическое имя «по умолчанию» для группы 0 не является встроенным в iproute2;
//он исходит из файла конфигурации группы по умолчанию. Вы можете добавить свои собственные, по одному на строку, следуя тому же 
//${number} ${name} формату. Вы можете иметь до 255 именованных групп.
//После того как вы настроили имя группы, ее номер и имя можно использовать в ip командах взаимозаменяемо.
//Пример:
//echo "10    customer-vlans" >> /etc/iproute2/group
//После этого вы можете использовать это имя во всех операциях, например:
//ip link set dev eth0.100 group customer-vlans
//TODO: Выполнить операцию над группой ip link set group ${group number} ${operation and arguments}
//Примеры:
//ip link set group 42 down
//ip link set group uplinks mtu 1200
//TODO: Просмотр информации о ссылках из определенной группы - обычная команда просмотра информации с group ${group}модификатором.
//Примеры:
//ip link list group 42
//ip address show group customers

//TODO: TUN и TAP устройства
//Устройства TUN и TAP позволяют программам пользовательского пространства эмулировать сетевое устройство.
//Разница между ними заключается в том, что устройства TAP работают с кадрами Ethernet (устройство L2), а TUN работает с IP-пакетами (устройство L3).
//Существует два типа устройств TUN/TAP: постоянные и временные. Временные устройства TUN/TAP создаются программами пользовательского пространства,
//когда они открывают специальное устройство, и автоматически уничтожаются при закрытии соответствующего файлового дескриптора.
//Постоянные устройства создаются с помощью ip команд, описанных ниже.
//TODO: Просмотр устройств TUN/TAP ip tuntap show
//TODO: Добавить устройство TUN/TAP, которое может использовать пользователь root ip tuntap add dev ${interface name} mode ${mode}
//Примеры:
//ip tuntap add dev tun0 mode tun
//ip tuntap add dev tap9 mode tap
//TODO: Добавить устройство TUN/TAP, которое может использовать обычный пользователь ip tuntap add dev ${interface name} mode ${mode} user ${user} group ${group}
//Пример:
//ip tuntap add dev tun1 mode tun user me group mygroup
//ip tuntap add dev tun2 mode tun user 1000 group 1001
//TODO: Добавить устройство TUN/TAP, используя альтернативный формат пакета ip tuntap add dev ${interface name} mode ${mode} pi
//Пример: ip tuntap add dev tun1 mode tun pi
//Добавьте метаинформацию к каждому пакету, полученному через файловый дескриптор.
//Очень немногие программы ожидают эту информацию, а программы, которые ее не ожидают, не будут правильно работать с устройством в этом режиме.
//TODO: Добавить TUN/TAP, игнорирование управления потоком ip tuntap add dev ${interface name} mode ${mode} one_queue
//Пример:ip tuntap add dev tun1 mode tun one_queue
//Обычно пакеты, отправляемые на устройство TUN/TAP, перемещаются так же, как и пакеты, отправляемые на любое другое устройство:
//   они помещаются в очередь, обрабатываемую механизмом управления трафиком (который настраивается командой) tc.
//   Это можно обойти, тем самым отключив механизм управления трафиком для этого устройства TUN/TAP.
//TODO: Удалить устройство TUN/TAP ip tuntap delete dev ${interface name} mode ${mode}
//Примеры:
//ip tuntap delete dev tun0 mode tun
//ip tuntap delete dev tap1 mode tap
//Примечание: необходимо указать режим. Режим не отображается в ip link show, поэтому, если вы не знаете, 
//является ли это устройством TUN или TAP, обратитесь к выходным данным ip tuntap show.

//TODO: Управление туннелем
//В настоящее время Linux поддерживает IPIP (IPv4 в IPv4), SIT (IPv6 в IPv4), IP6IP6 (IPv6 в IPv6), IPIP6 (IPv4 в IPv6),
//GRE (практически все в чем угодно) и VTI (IPv4 в IPsec).
//Обратите внимание, что туннели создаются в состоянии DOWN; вам нужно их поднять.
//TODO: Создаем туннель IPIP ip tunnel add ${interface name} mode ipip local ${local endpoint address} remote ${remote endpoint address}
//${local endpoint address}и ${remote endpoint address} относятся к адресам, назначенным физическим интерфейсам,
//а ${address}относятся к адресу, назначенному туннельному интерфейсу.
//Примеры:
//ip tunnel add tun0 mode ipip local 192.0.2.1 remote 198.51.100.3
//ip link set dev tun0 up
//ip address add 10.0.0.1/30 dev tun0
//TODO: Создать туннель SIT (6in4) ip tunnel add ${interface name} mode sit local ${local endpoint address} remote ${remote endpoint address}
//Примеры:
//ip tunnel add tun9 mode sit local 192.0.2.1 remote 198.51.100.3
//ip link set dev tun9 up
//ip address add 2001:db8:1::1/64 dev tun9
//Этот тип туннеля обычно используется для предоставления сети, подключенной к IPv4, с подключением IPv6.
//Существуют так называемые «туннельные брокеры», которые предоставляют его всем желающим, например, Tunnelbroker.net компании Hurricane Electric.
//TODO: Создать туннель IPIP6 ip -6 tunnel add ${interface name} mode ipip6 local ${local endpoint address} remote ${remote endpoint address}
//Примеры:ip -6 tunnel add tun8 mode ipip6 local 2001:db8:1::1 remote 2001:db8:1::2
//Этот тип туннеля будет широко использоваться только тогда, когда транзитные операторы постепенно откажутся от IPv4 (т. е. не в ближайшее время).
//TODO: Создаем туннель IP6IP6 ip -6 tunnel add ${interface name} mode ip6ip6 local ${local endpoint address} remote ${remote endpoint address}
//Примеры:
//ip -6 tunnel add tun3 mode ip6ip6 local 2001:db8:1::1 remote 2001:db8:1::2
//ip link set dev tun3 up
//ip address add 2001:db8:2:2::1/64 dev tun3
//Так же, как IPIP6, они не будут широко использоваться в ближайшее время.
//TODO: Создать туннельное устройство L2 GRE
//Статические туннели GRE традиционно используются для инкапсуляции пакетов IPv4 или IPv6, 
//но RFC не ограничивает полезную нагрузку GRE пакетами протокола L3. Можно инкапсулировать что угодно, включая кадры Ethernet.
//Однако в Linux GRE инкапсуляция относится конкретно к устройствам L3, тогда как для устройства L2,
//способного передавать кадры Ethernet, необходимо использовать инкапсуляцию gretap.
//TODO: # L2 GRE over IPv4 ip link add ${interface name} type gretap local ${local endpoint address} remote ${remote endpoint address}
//TODO: # L2 GRE over IPv6 ip link add ${interface name} type ip6gretap local ${local endpoint address} remote ${remote endpoint address}
//Эти туннели могут быть соединены мостом с другими физическими и виртуальными интерфейсами.
//Примеры:
//ip link add gretap0 type gretap local 192.0.2.1 remote 203.0.113.3
//ip link add gretap1 type ip6gretap local 2001:db8:dead::1 remote 2001:db8:beef::2
//TODO: Создать GRE-туннель ip tunnel add ${interface name} mode gre local ${local endpoint address} remote ${remote endpoint address}
//Примеры:
//ip tunnel add tun6 mode gre local 192.0.2.1 remote 203.0.113.3
//ip link set dev tun6 up
//ip address add 192.168.0.1/30 dev tun6
//ip address add 2001:db8:1::1/64 dev tun6
//GRE может инкапсулировать как IPv4, так и IPv6 одновременно. 
//Однако по умолчанию для транспорта используется IPv4, для GRE поверх IPv6 есть отдельный режим туннеля, ip6gre.
//TODO: Создать несколько туннелей GRE к одной и той же конечной точке ip tunnel add ${interface name} mode gre local ${local endpoint address} remote ${remote endpoint address} key ${key value}
//RFC2890 определяет туннели GRE с ключом. «Ключ» в данном случае не имеет ничего общего с шифрованием; это просто идентификатор,
//который позволяет маршрутизаторам отличать один туннель от другого, поэтому вы можете создавать несколько туннелей между одними и теми же конечными точками.
//Примеры:
//ip tunnel add tun4 mode gre local 192.0.2.1 remote 203.0.113.6 key 123
//ip tunnel add tun5 mode gre local 192.0.2.1 remote 203.0.113.6 key 124
//Вы также можете указать ключи в десятичном формате с точками, подобном IPv4.
//TODO: Создать туннель GRE типа "point-to-multipoint" ip tunnel add ${interface name} mode gre local ${local endpoint address} key ${key value}
//Примеры:
//ip tunnel add tun8 mode gre local 192.0.2.1 key 1234
//ip link set dev tun8 up
//ip address add 10.0.0.1/27 dev tun8
//Обратите внимание на отсутствие ${remote endpoint address}. Это то же самое, что и «multipoint режим gre» в Cisco IOS.
//В отсутствие адреса удаленной конечной точки ключ является единственным способом идентификации туннельного трафика, поэтому ${key value} необходим.
//Этот тип туннеля позволяет вам взаимодействовать с несколькими конечными точками, используя один и тот же туннельный интерфейс. 
//Он обычно используется в сложных настройках VPN с несколькими конечными точками, взаимодействующими друг с другом 
//(в терминологии Cisco, «dynamic multipoint VPN»).
//Поскольку явного адреса удаленной конечной точки нет, очевидно, что просто создать туннель недостаточно. 
//Ваша система должна знать, где находятся другие конечные точки. В реальной жизни для этого используется NHRP (протокол разрешения следующего перехода).
//Для тестирования вы можете добавить одноранговые узлы вручную
// (данная удаленная конечная точка использует адрес 203.0.113.6 на своем физическом интерфейсе и 10.0.0.2 на туннеле):
//ip neighbor add 10.0.0.2 lladdr 203.0.113.6 dev tun8
//Вам также придется сделать это на удаленной конечной точке:
//ip neighbor add 10.0.0.1 lladdr 192.0.2.1 dev tun8
//TODO: Создать туннель GRE через IPv6 ip -6 tunnel add name ${interface name} mode ip6gre local ${local endpoint} remote ${remote endpoint}
//Последние версии ядра и iproute2 поддерживают GRE через IPv6. Точка-точка без ключа:
//Он должен поддерживать все параметры и функции, поддерживаемые IPv4 GRE, описанные выше.
//TODO: Удалить туннель ip tunnel del ${interface name}
//Примеры:ip tunnel del gre1
//Обратите внимание, что в старых версиях iproute2 эта команда не поддерживала полный delete синтаксис, только del.
//В последних версиях разрешены как полные, так и сокращенные формы (проверено в iproute2-ss131122).
//TODO: Изменить туннель ip tunnel change ${interface name} ${options}
//Примеры:
//ip tunnel change tun0 remote 203.0.113.89
//ip tunnel change tun10 key 23456
//Примечание. Судя по всему, вы не можете добавить ключ к ранее не используемому туннелю. 
//Не уверен, баг это или фича. Кроме того, вы не можете изменить режим туннеля на лету по понятным причинам.
//TODO: Просмотр информации о туннеле ip tunnel show или ip tunnel show ${interface name}
//Примеры:
//$ ip tun show tun99
//tun99: gre/ip  remote 10.46.1.20  local 10.91.19.110  ttl inherit 

//TODO: Управление L2TPv3 pseudowire 
// L2TPv3 — это протокол туннелирования
//Во многих дистрибутивах L2TPv3 скомпилирован как модуль и может не загружаться по умолчанию. 
//Если выполнение какой-либо ip l2tp команды приводит к ошибкам, таким как RTNETLINK answers: No such file or directoryи Error talking to the kernel,
//вам необходимо загрузить l2tp_netlink и l2tp_eth модули ядра. 
//Если вы хотите использовать L2TPv3 по IP, а не по UDP, также загрузите файлы l2tp_ip.
//По сравнению с другими реализациями протокола туннелирования в Linux, терминология L2TPv3 несколько устарела.
//Вы создаете туннель, а затем привязываете к нему сеансы. К одному и тому же туннелю можно привязать несколько сеансов с разными идентификаторами.
//Виртуальные сетевые интерфейсы (по умолчанию именуемые l2tpethX) связаны с сеансами, а не с туннелями .
//Примечание
//Вы можете создавать только статические (неуправляемые) туннели L2TPv3 с помощью iproute2.
//Если вы хотите использовать L2TP для VPN с удаленным доступом или иным образом нуждаетесь в динамически создаваемых псевдопроводах,
//вам нужен демон пользовательского пространства для обработки этого.
//TODO: Создаем туннель L2TPv3 через UDP
//ip l2tp add tunnel
//tunnel_id ${local tunnel numeric identifier}
//peer_tunnel_id ${remote tunnel numeric identifier}
//udp_sport ${source port}
//udp_dport ${destination port}
//encap udp
//local ${local endpoint address}
//remote ${remote endpoint address}
//Примеры:
//ip l2tp add tunnel
//tunnel_id 1
//peer_tunnel_id 1
//udp_sport 5000
//udp_dport 5000 
//encap udp
//local 192.0.2.1
//remote 203.0.113.2
//Примечание. Идентификаторы туннеля и другие параметры на обеих конечных точках должны совпадать.
//TODO: Создаем туннель L2TPv3 через IP
//ip l2tp add tunnel
//tunnel_id ${local tunnel numeric identifier}
//peer_tunnel_id {remote tunnel numeric identifier }
//encap ip
//local 192.0.2.1
//remote 203.0.113.2
//Инкапсуляция кадров L2TPv3 в IP, а не в UDP, создает меньшие накладные расходы,
//но может вызвать проблемы для конечных точек за NAT.
//TODO: Создать сессию L2TPv3
//ip l2tp add session tunnel_id ${local tunnel identifier}
//session_id ${local session numeric identifier}
//peer_session_id ${remote session numeric identifier}
//Примеры:
//ip l2tp add session tunnel_id 1 session_id 10 peer_session_id 10
//Примечания: tunnel_id значение должно соответствовать значению существующего туннеля (iproute2 не создаст туннель, если он не существует).
//Идентификаторы сеансов на обеих конечных точках должны совпадать.
//Как только вы создадите туннель и сеанс, l2tpethX интерфейс появится в состоянии DOWN. 
//Измените состояние на UP и соедините его с другим интерфейсом или назначьте ему адрес.
//TODO: Удалить сеанс L2TPv3 ip l2tp del session tunnel_id ${tunnel identifier} session_id ${session identifier}
//Примеры:
//ip l2tp del session tunnel_id 1 session_id 1
//TODOЖ Удалить туннель L2TPv3 ip l2tp del tunnel tunnel_id ${tunnel identifier}
//Примеры:
//ip l2tp del tunnel tunnel_id 1
//Примечание. Перед удалением самого туннеля необходимо удалить все сеансы, связанные с туннелем.
//TODO: Просмотр информации о туннеле L2TPv3
//ip l2tp show tunnel
//ip l2tp show tunnel tunnel_id ${tunnel identifier}
//Примеры:
//ip l2tp show tunnel tunnel_id 12
//TODO: Просмотр информации о сеансе L2TPv3
//ip l2tp show session
//ip l2tp show session session_id ${session identifier}
//tunnel_id ${tunnel identifier}
//Примеры:
//ip l2tp show session session_id 1 tunnel_id 12

//TODO: Управление VXLAN
//VXLAN — это протокол туннелирования, разработанный для распределенных коммутируемых сетей. Он часто используется в настройках виртуализации,
//чтобы отделить топологию виртуальной сети от топологии базовой физической сети.
//VXLAN может работать как в многоадресном, так и в одноадресном режиме и поддерживает изоляцию виртуальных сетей
//с помощью VNI (идентификатор виртуальной сети), аналогично VLAN в сетях Ethernet.
//Недостатком многоадресного режима является то, что вам нужно будет использовать протокол многоадресной маршрутизации, 
//обычно PIM-SM, чтобы заставить его работать в маршрутизируемых сетях, но если вы его настроите, вам не нужно создавать все соединения VXLAN. рукой.
//Базовым протоколом инкапсуляции для VXLAN является UDP.
//TODO: Создать одноадресную ссылку VXLAN
//ip link add name ${interface name} type vxlan 
//   id <0-16777215> 
//   dev ${source interface}
//   remote ${remote endpoint address} 
//   local ${local endpoint address} 
//   dstport ${VXLAN destination port}
//Пример:
//ip link add name vxlan0 type vxlan 
//   id 42 dev eth0 remote 203.0.113.6 local 192.0.2.1 dstport 4789
//Примечание. id параметр — сетевой идентификатор VXLAN (VNI).
//TODO: Создать многоадресную ссылку VXLAN
//ip link add name ${interface name} type vxlan 
//   id <0-16777215> 
//   dev ${source interface} 
//   group ${multicast address} 
//   dstport ${VXLAN destination port}
//Пример:
//ip link add name vxlan0 type vxlan 
//   id 42 dev eth0 group 239.0.0.1 dstport 4789
//После этого нужно поднять линк и либо соединить его с другим интерфейсом, либо присвоить ему адрес.


#define SETMULTICASTON_CMD     "ip link set dev %S multicast on"
#define SETMULTICASTOFF_CMD    "ip link set dev %S multicast off"

#define SETALLMULTICASTON_CMD  "ip link set dev %S allmulticast on"
#define SETALLMULTICASTOFF_CMD "ip link set dev %S allmulticast off"

// Можно отключить ARP, чтобы применить политику безопасности и разрешить только определенным MAC-адресам взаимодействовать с интерфейсом.
// В этом случае записи таблицы соседей для MAC-адресов из белого списка должны быть созданы вручную, иначе ничто не сможет взаимодействовать с этим интерфейсом.
#define SETARPON_CMD           "ip link set dev %S arp off"
#define SETARPOFF_CMD          "ip link set dev %S arp on"

#define CHANGEIFNAME_CMD  "ip link set dev %S down && ip link set dev %S name %S && ip link set dev %S up"
#define SETDOWN_CMD       "ip link set dev %S down"
#define SETUP_CMD         "ip link set dev %S up"
#define CHANGEMAC_CMD     "ip link set dev %S down && ip link set dev %S address %S && ip link set dev %S up"
#define SETMTU_CMD        "ip link set dev %S mtu %u"
#define ADDIP_CMD         "ip address add %S/%u broadcast %S dev %S"
#define DELETEIP_CMD      "ip address del %S/%u dev %S"
#define CHANGEIP_CMD      "ip address del %S/%u dev %S && ip address add %S/%u broadcast %S dev %S"
#define DELETEIP6_CMD     "ip -6 address del %S/%u dev %S"
#define ADDIP6_CMD        "ip -6 address add %S/%u dev %S"
#define CHANGEIP6_CMD     "ip -6 address del %S/%u dev %S && ip -6 address add %S/%u dev %S"
#define SETPROMISCON_CMD  "ip link set dev %S promisc on"
#define SETPROMISCOFF_CMD "ip link set dev %S promisc off"

#else

#define CHANGEIFNAME_CMD       "change ifname unsupported on macos: %S %S %S %S"
#define SETMULTICASTON_CMD     "set multicast on unsupported on macos: %S"
#define SETMULTICASTOFF_CMD    "set multicast off unsupported on macos: %S"

#define SETALLMULTICASTON_CMD  "set allmulticast on unsupported on macos: %S"
#define SETALLMULTICASTOFF_CMD "set allmulticast off unsupported on macos: %S"

#define SETARPON_CMD           "set arp on unsupported on macos: %S"
#define SETARPOFF_CMD          "set arp off unsupported on macos: %S"

#define SETDOWN_CMD       "ifconfig %S down"
#define SETUP_CMD         "ifconfig %S up"

#define CHANGEMAC_CMD     "ifconfig %S ether %S && ifconfig %S down && ifconfig %S up"
#define SETMTU_CMD        "ifconfig %S mtu %u"
#define ADDIP_CMD         "ifconfig %S inet %S/%u broadcast %S add"
#define DELETEIP_CMD      "ifconfig %S inet %S/%u delete"
#define CHANGEIP_CMD      "ifconfig %S inet %S/%u delete && ifconfig %S inet %S/%u broadcast %S add"

#define ADDIP6_CMD        "ifconfig %S inet6 %S/%u add"
#define DELETEIP6_CMD     "ifconfig %S inet6 %S/%u delete"
#define CHANGEIP6_CMD     "ifconfig %S inet6 %S/%u delete && ifconfig %S inet6 %S/%u add"

#define SETPROMISCON_CMD  "ifconfig %S promisc"
#define SETPROMISCOFF_CMD "ifconfig %S -promisc"

#endif
