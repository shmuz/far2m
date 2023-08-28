m4_include(`farversion.m4')m4_dnl
#hpp file name
lang.inc
#number of languages
8
#id:0 language file name, language name, language description
FarRus.lng Russian "Russian (Русский)"
#id:1 language file name, language name, language description
FarEng.lng English "English"
#id:2 language file name, language name, language description
FarCze.lng Czech "Czech (Čeština)"
#id:3 language file name, language name, language description
FarGer.lng German "German (Deutsch)"
#id:4 language file name, language name, language description
FarHun.lng Hungarian "Hungarian (Magyar)"
#id:5 language file name, language name, language description
FarPol.lng Polish "Polish (Polski)"
#id:6 language file name, language name, language description
FarSpa.lng Spanish "Spanish (Español)"
#id:7 language file name, language name, language description
FarUkr.lng Ukrainian "Ukrainian (Український)"

#head of the hpp file
#hhead:
#hhead:

#tail of the hpp file
#htail:
#htail:
#and so on as much as needed

enum:LangMsg

#--------------------------------------------------------------------
#now come the lng feeds
#--------------------------------------------------------------------
#first comes the text name from the enum which can be preceded with
#comments that will go to the hpp file
#h://This comment will appear before Yes
#he://This comment will appear after Yes
#Yes
#now come the lng lines for all the languages in the order defined
#above, they can be preceded with comments as shown below
#l://This comment will appear in all the lng files before the lng line
#le://This comment will appear in all the lng files after the lng line
#ls://This comment will appear only in Russian lng file before the lng line
#lse://This comment will appear only in Russian lng file after the lng line
#"Да"
#ls://This comment will appear only in English lng file before the lng line
#lse://This comment will appear only in English lng file after the lng line
#"Yes"
#ls://This comment will appear only in Czech lng file before the lng line
#lse://This comment will appear only in Czech lng file after the lng line
#upd:"Ano"
#
#lng lines marked with "upd:" will cause a warning to be printed to the
#screen reminding that this line should be updated/translated


Yes
`l://Version: 'MAJOR`.'MINOR` build 'PATCH
l:
"Да"
"Yes"
"Ano"
"Ja"
"Igen"
"Tak"
"Si"
"Так"

No
"Нет"
"No"
"Ne"
"Nein"
"Nem"
"Nie"
"No"
"Ні"

Ok
"OK"
"OK"
"Ok"
"OK"
"OK"
"OK"
"Aceptar"
"OK"

HYes
l:
"&Да"
"&Yes"
"&Ano"
"&Ja"
"I&gen"
"&Tak"
"&Si"
"&Так"

HNo
"&Нет"
"&No"
"&Ne"
"&Nein"
"Ne&m"
"&Nie"
"&No"
"&Ні"

HOk
"&OK"
"&OK"
"&Ok"
"&OK"
"&OK"
"&OK"
"&Aceptar"
"&OK"

Cancel
l:
"Отмена"
"Cancel"
"Storno"
"Abbrechen"
"Mégsem"
"Anuluj"
"Cancelar"
"Відміна"

Retry
"Повторить"
"Retry"
"Znovu"
"Wiederholen"
"Újra"
"Ponów"
"Reiterar"
"Повторити"

Skip
"Пропустить"
"Skip"
"Přeskočit"
"Überspringen"
"Kihagy"
"Omiń"
"Omitir"
"Пропустити"

Abort
"Прервать"
"Abort"
"Zrušit"
"Abbrechen"
"Megszakít"
"Zaniechaj"
"Abortar"
"Перервати"

Ignore
"Игнорировать"
"Ignore"
"Ignorovat"
"Ignorieren"
"Mégis"
"Zignoruj"
"Ignorar"
"Ігнорувати"

Delete
"Удалить"
"Delete"
"Smazat"
"Löschen"
"Töröl"
"Usuń"
"Borrar"
"Вилучити"

HCancel
l:
"&Отмена"
"&Cancel"
"&Storno"
"&Abbrechen"
"Még&sem"
"&Anuluj"
"&Cancelar"
"&Відміна"

HRetry
"&Повторить"
"&Retry"
"&Znovu"
"&Wiederholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

HSkip
"П&ропустить"
"&Skip"
"&Přeskočit"
"Über&springen"
"Ki&hagy"
"&Omiń"
"&Omitir"
"П&ропустити"

HSkipAll
"Пропустить &все"
"S&kip all"
"Přeskočit &vše"
"Alle übersprin&gen"
"Kihagy &mind"
"Omiń &wszystkie"
"Omitir &Todo"
"Пропустити &усе"

Warning
l:
"Предупреждение"
"Warning"
"Varování"
"Warnung"
"Figyelem"
"Ostrzeżenie"
"Advertencia"
"Попередження"

Error
"Ошибка"
"Error"
"Chyba"
"Fehler"
"Hiba"
"Błąd"
"Error"
"Помилка"

Quit
l:
"Выход"
"Quit"
"Konec"
"Beenden"
"Kilépés"
"Zakończ"
"Salir"
"Вихід"

AskQuit
"Вы хотите завершить работу в FAR?"
"Do you want to quit FAR?"
"Opravdu chcete ukončit FAR?"
"Wollen Sie FAR beenden?"
"Biztosan kilép a FAR-ból?"
"Czy chcesz zakończyć pracę z FARem?"
"Desea salir de FAR?"
"Ви хочете завершити роботу в FAR?"

Background
"&В фон"
"&Background"
"&Background"
"&Background"
"&Background"
"&Background"
"&Background"
"&У тлі"

GetOut
"&Выбраться"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Вибратися"

F1
l:
l://functional keys - 6 characters max
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

F2
"ПользМ"
"UserMn"
"UživMn"
"BenuMn"
"FhMenü"
"Menu"
"Menú "
"КористМ"

AltF1
l:
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda "
"Ліва"

AltF8
"Истор"
"Histry"
"Histor"
"Histor"
"ParElő"
"Histor"
"Histor"
"Істор"

CtrlF1
l:
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda "
"Ліва"

ShiftF1
l:
"Добавл"
"Add"
"Přidat"
"Hinzu"
"Tömört"
"Dodaj"
"Añadir"
"Добавл"

AltShiftF1
l:
l:// Main AltShift
""
""
""
""
""
""
""
""

CtrlShiftF1
l:
l://Main CtrlShift
""
""
""
""
""
""
""
""

CtrlAltF1
l:
l:// Main CtrlAlt
""
""
""
""
""
""
""
""

CtrlAltShiftF1
l:
l:// Main CtrlAltShift
""
""
""
""
""
""
""
""

HistoryTitle
l:
"История команд"
"History"
"Historie"
"Historie der letzten Befehle"
"Parancs előzmények"
"Historia"
"Historial"
"Історія команд"

FolderHistoryTitle
"История папок"
"Folders history"
"Historie adresářů"
"Zuletzt besuchte Ordner"
"Mappa előzmények"
"Historia katalogów"
"Historial directorios"
"Історія тек"

ViewHistoryTitle
"История просмотра"
"File view history"
"Historie prohlížení souborů"
"Zuletzt betrachtete Dateien"
"Fájl előzmények"
"Historia podglądu plików"
"Historial visor"
"Історія перегляду"

ViewHistoryIsCreate
"Создать файл?"
"Create file?"
"Vytvořit soubor?"
"Datei erstellen?"
"Fájl létrehozása?"
"Utworzyć plik?"
"Crear archivo?"
"Створити файл?"

HistoryView
"Просмотр"
"View"
"Zobrazit"
"Betr"
"Nézett"
"Zobacz"
"Ver   "
"Перегляд"

HistoryEdit
"Редактор"
"Edit"
"Editovat"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редактор"

HistoryExt
"Внешний"
"Ext."
"Rozšíření"
"Ext."
"Kit."
"Ext."
"Ext."
"Зовнішній"

HistoryClear
l:
"История будет полностью очищена. Продолжить?"
"All records in the history will be deleted. Continue?"
"Všechny záznamy v historii budou smazány. Pokračovat?"
"Die gesamte Historie wird gelöscht. Fortfahren?"
"Az előzmények minden eleme törlődik. Folytatja?"
"Wszystkie wpisy historii będą usunięte. Kontynuować?"
"Los datos en el historial serán borrados. Continuar?"
"Історія буде повністю очищена. Продовжити?"

Clear
"&Очистить"
"&Clear history"
"&Vymazat historii"
"Historie &löschen"
"Elő&zmények törlése"
"&Czyść historię"
"&Limpiar historial"
"&Очистити"

ConfigSystemTitle
l:
"Системные параметры"
"System settings"
"Nastavení systému"
"Grundeinstellungen"
"Rendszer beállítások"
"Ustawienia systemowe"
"Opciones de sistema"
"Системні параметри"

ConfigSudoEnabled
"Разрешить повышение привилегий"
"Enable s&udo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
"Дозволити підвишення привілеїв"

ConfigSudoConfirmModify
"Подтверждать все операции записи"
"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
"Підтверджувати усі операції запису"

ConfigSudoPasswordExpiration
"Время действия пароля (сек):"
"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
"Час дії пароля (сек):"

SudoTitle
"Операция требует повышения привилегий"
"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
"Операція вимагає підвищення привілеїв"

SudoPrompt
"Введите пароль для sudo"
"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
"Введіть пароль для sudo"

SudoConfirm
"Подтвердите использование привилегий"
"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
"Підтвердіть використання привілеїв"

ConfigRecycleBin
"Удалять в &Корзину"
"&Delete to Recycle Bin"
"&Mazat do Koše"
"In Papierkorb &löschen"
"&Törlés a Lomtárba"
"&Usuwaj do Kosza"
"Borrar hacia &papelera de reciclaje"
"Видаляти у &Кошик"

ConfigRecycleBinLink
"У&далять символические ссылки"
"Delete symbolic &links"
"Mazat symbolické &linky"
"Symbolische L&inks löschen"
"Szimbolikus l&inkek törlése"
"Usuń &linki symboliczne"
"Borrar en&laces simbólicos"
"В&идаляти символічні посилання"

CopyWriteThrough
"Выключить кэ&ширование записи"
"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
"Вимкнути ке&шування запису"

CopyXAttr
"Копировать расширенные а&ттрибуты"
"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
"Копіювати розширені а&трибути"

ConfigOnlyFilesSize
"Учитывать только размер файлов"
"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
"Враховувати лише розмір файлів"

ConfigScanJunction
"Ск&анировать символические ссылки"
"Scan s&ymbolic links"
"Prohledávat s&ymbolické linky"
"S&ymbolische Links scannen"
"Szimbolikus linkek &vizsgálata"
"Skanuj linki s&ymboliczne"
"Explorar enlaces simbólicos"
"Ск&анувати символічні посилання"

ConfigInactivity
"&Время бездействия"
"&Inactivity time"
"&Doba nečinnosti"
"Inaktivitäts&zeit"
"A FAR kilé&p"
"Czas &bezczynności"
"Desact&ivar FAR en..."
"&Час бездіяльності"

ConfigInactivityMinutes
"минут"
"minutes"
"minut"
"Minuten"
"perc tétlenség után"
"&minut"
"minutos"
"хвилин"

ConfigSaveHistory
"Сохранять &историю команд"
"Save commands &history"
"Ukládat historii &příkazů"
"&Befehlshistorie speichern"
"Parancs elő&zmények mentése"
"Zapisz historię &poleceń"
"Guardar &historial de comandos"
"Зберігати &історію команд"

ConfigSaveFoldersHistory
"Сохранять историю п&апок"
"Save &folders history"
"Ukládat historii &adresářů"
"&Ordnerhistorie speichern"
"M&appa előzmények mentése"
"Zapisz historię &katalogów"
"Guardar historial de directorios"
"Зберігати історію т&ек"

ConfigSaveViewHistory
"Сохранять историю п&росмотра и редактора"
"Save &view and edit history"
"Ukládat historii Zobraz a Editu&j"
"Betrachter/&Editor-Historie speichern"
"Nézőke és &szerkesztő előzmények mentése"
"Zapisz historię podglądu i &edycji"
"Guardar historial de &visor y editor"
"Зберігати історію п&ерегляду та редагування"

ConfigAutoSave
"Автозапись кон&фигурации"
"Auto &save setup"
"Automatické ukládaní &nastavení"
"Setup automatisch &"speichern"
"B&eállítások automatikus mentése"
"Automatycznie &zapisuj ustawienia"
"Auto&guardar configuración"
"Автозапис кон&фігурації"

ConfigPanelTitle
l:
"Настройки панели"
"Panel settings"
"Nastavení panelů"
"Panels einrichten"
"Panel beállítások"
"Ustawienia panelu"
"Configuración de paneles"
"Налаштування панели"

ConfigHidden
"Показывать скр&ытые и системные файлы"
"Show &hidden and system files"
"Ukázat &skryté a systémové soubory"
"&Versteckte und Systemdateien anzeigen"
"&Rejtett és rendszerfájlok mutatva"
"Pokazuj pliki &ukryte i systemowe"
"Mostrar archivos ocultos y de sistema"
"Показувати при&ховані та системні файли"

ConfigHighlight
"&Раскраска файлов"
"Hi&ghlight files"
"Zvý&razňovat soubory"
"Dateien mark&ieren"
"Fá&jlok kiemelése"
"W&yróżniaj pliki"
"Resaltar archivos"
"&Розфарбовка файлів"

ConfigAutoChange
"&Автосмена папки"
"&Auto change folder"
"&Automaticky měnit adresář"
"Ordner &automatisch wechseln (Baumansicht)"
"&Automatikus mappaváltás"
"&Automatycznie zmieniaj katalog"
"&Auto cambiar directorio"
"&Автозміна теки"

ConfigSelectFolders
"Пометка &папок"
"Select &folders"
"Vybírat a&dresáře"
"&Ordner auswählen"
"A ma&ppák is kijelölhetők"
"Zaznaczaj katalo&gi"
"Seleccionar &directorios"
"Позначка &тек"

ConfigSortFolderExt
"Сортировать имена папок по рас&ширению"
"Sort folder names by e&xtension"
"Řadit adresáře podle přípony"
"Ordner nach Er&weiterung sortieren"
"Mappák is rendezhetők &kiterjesztés szerint"
"Sortuj nazwy katalogów wg r&ozszerzeń"
"Ordenar directorios por extensión"
"Сортувати імена папок по роз&ширенню"

ConfigReverseSort
"Разрешить &обратную сортировку"
"Allow re&verse sort modes"
"Do&volit změnu směru řazení"
"&Umgekehrte Sortiermodi zulassen"
"Fordí&tott rendezés engedélyezése"
"Włącz &możliwość odwrotnego sortowania"
"Permitir modo de orden in&verso"
"Дозволити &зворотне сортування"

ConfigAutoUpdateLimit
"Отключать автооб&новление панелей,"
"&Disable automatic update of panels"
"Vypnout a&utomatickou aktualizaci panelů"
"Automatisches Panelupdate &deaktivieren"
"Pan&el automatikus frissítése kikapcsolva,"
"&Wyłącz automatyczną aktualizację paneli"
"Desactiva actualización automát. de &paneles"
"Відключати автоо&новлення панелей,"

ConfigAutoUpdateLimit2
"если объектов больше"
"if object count exceeds"
"jestliže počet objektů překročí"
"wenn mehr Objekte als"
"ha több elem van, mint:"
"jeśli zawierają więcej obiektów niż"
"si conteo de objetos es excedido"
"якщо об'ектів більше"

ConfigAutoUpdateRemoteDrive
"Автообновление с&етевых дисков"
"Network drives autor&efresh"
"Automatická obnova síťových disků"
"Netzw&erklauferke autom. aktualisieren"
"Hálózati meghajtók autom. &frissítése"
"Auto&odświeżanie dysków sieciowych"
"Autor&efrescar unidades de Red"
"Автооновлення м&ережевих дисків"

ConfigShowColumns
"Показывать &заголовки колонок"
"Show &column titles"
"Zobrazovat &nadpisy sloupců"
"S&paltentitel anzeigen"
"Oszlop&nevek mutatva"
"Wyświetl tytuły &kolumn"
"Mostrar títulos de &columnas"
"Показувати &заголовки колонок"

ConfigShowStatus
"Показывать &строку статуса"
"Show &status line"
"Zobrazovat sta&vový řádek"
"&Statuszeile anzeigen"
"Á&llapotsor mutatva"
"Wyświetl &linię statusu"
"Mostrar línea de e&stado"
"Показувати &рядок статусу"

ConfigShowTotal
"Показывать су&ммарную информацию"
"Show files &total information"
"Zobrazovat &informace o velikosti souborů"
"&Gesamtzahl für Dateien anzeigen"
"Fájl össze&s információja mutatva"
"Wyświetl &całkowitą informację o plikach"
"Mostrar información comple&ta de archivos"
"Показувати су&марну інформацію"

ConfigShowFree
"Показывать с&вободное место"
"Show f&ree size"
"Zobrazovat vo&lné místo"
"&Freien Speicher anzeigen"
"Sza&bad lemezterület mutatva"
"Wyświetl ilość &wolnego miejsca"
"Mostrar espacio lib&re"
"Показувати в&ільне місце"

ConfigShowScrollbar
"Показывать по&лосу прокрутки"
"Show scroll&bar"
"Zobrazovat &posuvník"
"Scroll&balken anzeigen"
"Gördítősá&v mutatva"
"Wyświetl &suwak"
"Mostrar &barra de desplazamiento"
"Показувати см&угу прокручування"

ConfigShowScreensNumber
"Показывать количество &фоновых экранов"
"Show background screens &number"
"Zobrazovat počet &obrazovek na pozadí"
"&Nummer von Hintergrundseiten anzeigen"
"&Háttérképernyők száma mutatva"
"Wyświetl ilość &ekranów w tle"
"Mostrar &número de pantallas de fondo"
"Показувати кількість &фонових екранів"

ConfigShowSortMode
"Показывать букву режима сор&тировки"
"Show sort &mode letter"
"Zobrazovat písmeno &módu řazení"
"Buchstaben der Sortier&modi anzeigen"
"Rendezési mó&d betűjele mutatva"
"Wyświetl l&iterę trybu sortowania"
"Mostrar letra para &modo de orden"
"Показувати літеру режиму сор&тування"

ConfigInterfaceTitle
l:
"Настройки интерфейса"
"Interface settings"
"Nastavení rozhraní"
"Oberfläche einrichten"
"Kezelőfelület beállítások"
"Ustawienia interfejsu"
"Opciones de interfaz"
"Налаштування інтерфейсу"

ConfigInputTitle
l:
"Настройки ввода"
"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
"Налаштування введення"

ConfigClock
"&Часы в панелях"
"&Clock in panels"
"&Hodiny v panelech"
"&Uhr in Panels anzeigen"
"Ór&a a paneleken"
"&Zegar"
"&Reloj en paneles"
"&Годинник у панелях"

ConfigViewerEditorClock
"Ч&асы при редактировании и просмотре"
"C&lock in viewer and editor"
"H&odiny v prohlížeči a editoru"
"U&hr in Betrachter und Editor anzeigen"
"Ó&ra a nézőkében és szerkesztőben"
"Zegar w &podglądzie i edytorze"
"Re&loj en visor y editor"
"Г&одинник під час редагування та перегляду"

ConfigMouse
"Мы&шь"
"M&ouse"
"M&yš"
"M&aus aktivieren"
"&Egér kezelése"
"M&ysz"
"Rat&ón"
"Ми&ша"

ConfigXLats
"Правила &транслитерации:"
"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
"Правила &транслітерації:"

ConfigXLatDialogs
"Транслитерация &диалоговой навигации"
"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Транслітерація &діалогової навігації"

ConfigXLatFastFileFind
"Транслитерация быстрого поиска &файла"
"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Транслітерація швидкого пошуку &файлу"

ConfigKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавіш"

ConfigMenuBar
"Всегда показывать &меню"
"Always show &menu bar"
"Vždy zobrazovat hlavní &menu"
"&Menüleiste immer anzeigen"
"A &menüsor mindig látszik"
"Zawsze pokazuj pasek &menu"
"Mostrar siempre barra de &menú"
"Завжди показувати &меню"

ConfigSaver
"&Сохранение экрана"
"&Screen saver"
"Sp&ořič obrazovky"
"Bildschirm&schoner"
"&Képernyőpihentető"
"&Wygaszacz ekranu"
"&Salvapantallas"
"&Збереження екрана"

ConfigSaverMinutes
"минут"
"minutes"
"minut"
"Minuten"
"perc tétlenség után"
"m&inut"
"minutos"
"хвилин"

ConfigConsoleChangeFont
"Выбрать шри&фт"
"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
"Вибрати шри&фт"

ConfigConsolePaintSharp
"Отключить сглаживание"
"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
"Відключити згладжування"

ConfigOSC52ClipSet
"Исп. OSC&52 для записи в буфер обмена"
"Use OSC&52 to set clipboard data"
upd:"Use OSC&52 to set clipboard data"
upd:"Use OSC&52 to set clipboard data"
upd:"Use OSC&52 to set clipboard data"
upd:"Use OSC&52 to set clipboard data"
upd:"Use OSC&52 to set clipboard data"
upd:"Use OSC&52 to set clipboard data"

ConfigTTYPaletteOverride
"Исп. свою &палитру базовых цветов"
"Override base colors &palette"
upd:"Override base colors &palette"
upd:"Override base colors &palette"
upd:"Override base colors &palette"
upd:"Override base colors &palette"
upd:"Override base colors &palette"
upd:"Override base colors &palette"

ConfigExclusiveKeys
"&Экслюзивная обработка нажатий, включающих:"
"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
"&Ексклюзивне оброблення натискань, що включають:"

ConfigExclusiveCtrlLeft
"Левый Ctrl"
"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
"Лівий Ctrl"

ConfigExclusiveCtrlRight
"Правый Ctrl"
"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
"Правий Ctrl"

ConfigExclusiveAltLeft
"Левый Alt "
"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
"Лівий Alt "

ConfigExclusiveAltRight
"Правый Alt "
"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
"Правий Alt "

ConfigExclusiveWinLeft
"Левый Win "
"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
"Лівий Win "

ConfigExclusiveWinRight
"Правый Win "
"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
"Правий Win "

ConfigCopyTotal
"Показывать &общий индикатор копирования"
"Show &total copy progress indicator"
"Zobraz. ukazatel celkového stavu &kopírování"
"Zeige Gesamtfor&tschritt beim Kopieren"
"Másolás összesen folyamat&jelző"
"Pokaż &całkowity postęp kopiowania"
"Mostrar indicador de progreso de copia &total"
"Показувати &загальний індикатор копіювання"

ConfigCopyTimeRule
"Показывать информацию о времени &копирования"
"Show cop&ying time information"
"Zobrazovat informace o čase kopírování"
"Zeige Rest&zeit beim Kopieren"
"Má&solási idő mutatva"
"Pokaż informację o c&zasie kopiowania"
"Mostrar información de tiempo de copiado"
"Показувати інформацію про час &копіювання"

ConfigDeleteTotal
"Показывать общий индикатор удаления"
"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
"Mostrar indicador de progreso de borrado total"
"Показувати загальний індикатор видалення"

ConfigPgUpChangeDisk
"Использовать Ctrl-PgUp для в&ыбора диска"
"Use Ctrl-Pg&Up to change drive"
"Použít Ctrl-Pg&Up pro změnu disku"
"Strg-Pg&Up wechselt das Laufwerk"
"A Ctrl-Pg&Up meghajtót vált"
"Użyj Ctrl-Pg&Up do zmiany napędu"
"Usar Ctrl-Pg&Up para cambiar unidad"
"Використовувати Ctrl-PgUp для в&ибору диска"

ConfigWindowTitle
"Заголовок окна FAR:"
"FAR window title:"
upd:"FAR window title:"
upd:"FAR window title:"
upd:"FAR window title:"
upd:"FAR window title:"
"Título de ventana de FAR:"
"Заголовок вікна FAR:"

ConfigDlgSetsTitle
l:
"Настройки диалогов"
"Dialog settings"
"Nastavení dialogů"
"Dialoge einrichten"
"Párbeszédablak beállítások"
"Ustawienia okien dialogowych"
"Opciones de diálogo"
"Налаштування діалогів"

ConfigDialogsEditHistory
"&История в строках ввода диалогов"
"&History in dialog edit controls"
"H&istorie v dialozích"
"&Historie in Eingabefelder von Dialogen"
"&Beviteli sor előzmények mentése"
"&Historia w polach edycyjnych"
"&Historial en controles de diálogo de edición"
"&Історія у рядках введення діалогів"

ConfigMaxHistoryCount
"&Макс количество записей:"
"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
"&Макс кількість записів:"

ConfigDialogsEditBlock
"&Постоянные блоки в строках ввода"
"&Persistent blocks in edit controls"
"&Trvalé bloky v editačních polích"
"Dauer&hafte Markierungen in Eingabefelder"
"Maradó b&lokkok a beviteli sorokban"
"&Trwałe bloki podczas edycji"
"&Bloques persistentes en controles de edición"
"&Постійні блоки в рядках введення"

ConfigDialogsDelRemovesBlocks
"Del удаляет б&локи в строках ввода"
"&Del removes blocks in edit controls"
"&Del maže položky v editačních polích"
"&Entf löscht Markierungen"
"A &Del törli a beviteli sorok blokkjait"
"&Del usuwa blok podczas edycji"
"&Del remueve bloques en controles de edición"
"Del видаляє б&локи в рядках введення"

ConfigDialogsAutoComplete
"&Автозавершение в строках ввода"
"&AutoComplete in edit controls"
"Automatické dokončování v editač&ních polích"
"&Automatisches Vervollständigen"
"Beviteli sor a&utomatikus kiegészítése"
"&Autouzupełnianie podczas edycji"
"Autocompl&etar en controles de edición"
"&Автозавершення в рядках введення"

ConfigDialogsEULBsClear
"Backspace &удаляет неизмененный текст"
"&Backspace deletes unchanged text"
"&Backspace maže nezměněný text"
"&Rücktaste (BS) löscht unveränderten Text"
"A Ba&ckspace törli a változatlan szöveget"
"&Backspace usuwa nie zmieniony tekst"
"&Backspace elimina texto no cambiado"
"Backspace &видаляє незмінений текст"

ConfigDialogsMouseButton
"Клик мыши &вне диалога закрывает диалог"
"Mouse click &outside a dialog closes it"
"Kl&iknutí myší mimo dialog ho zavře"
"Dial&og schließen wenn Mausklick ausserhalb"
"&Egérkattintás a párb.ablakon kívül: bezárja"
"&Kliknięcie myszy poza oknem zamyka je"
"Click en ratón afuera del diálogo lo cierra"
"Клік миши &поза діалогом закриває діалог"

ConfigVMenuTitle
l:
"Настройки меню"
"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
"Налаштування меню"

ConfigVMenuLBtnClick
"Клик левой кнопки мыши вне меню"
"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
"Клік лівої кнопки миши поза меню"

ConfigVMenuRBtnClick
"Клик правой кнопки мыши вне меню"
"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
"Клік правої кнопки миши поза меню"

ConfigVMenuMBtnClick
"Клик средней кнопки мыши вне меню"
"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
"Клік середньої кнопки миши поза меню"

ConfigVMenuClickCancel
"Закрыть с отменой"
"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
"Закрити зі скасуванням"

ConfigVMenuClickApply
"Выполнить текущий пункт"
"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
"Виконати поточний пункт"

ConfigVMenuClickIgnore
"Ничего не делать"
"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
"Нічого не робити"

ConfigCmdlineTitle
l:
"Настройки командной строки"
"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
"Opciones de línea de comando"
"Налаштування командного рядка"

ConfigCmdlineEditBlock
"&Постоянные блоки"
"&Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
"Bloques &persistentes"
"&Постійні блоки"

ConfigCmdlineDelRemovesBlocks
"Del удаляет б&локи"
"&Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
"&Del remueve bloques"
"Del видаляє б&локи"

ConfigCmdlineAutoComplete
"&Автозавершение"
"&AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
"&AutoCompletar"
"&Автозавершення"

ConfigCmdlineVTLogLimit
"Максимум строк в &логе вывода:"
"Maximum terminal &log lines:"
upd:"Maximum terminal &log lines:"
upd:"Maximum terminal &log lines:"
upd:"Maximum terminal &log lines:"
upd:"Maximum terminal &log lines:"
upd:"Maximum terminal &log lines:"
"Максимум рядків &логії виводу:"

ConfigCmdlineWaitKeypress
"&Ожидать нажатие перед закрытием"
"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
"&Очікувать натискання перед закриттям"

ConfigCmdlineWaitKeypress_Never
"Никогда"
"Never"
upd:"Never"
upd:"Never"
upd:"Never"
upd:"Never"
"Never"
"Ніколи"

ConfigCmdlineWaitKeypress_OnError
"При ошибке"
"On error"
upd:"On error"
upd:"On error"
upd:"On error"
upd:"On error"
"On error"
"При помилці"

ConfigCmdlineWaitKeypress_Always
"Всегда"
"Always"
upd:"Always"
upd:"Always"
upd:"Always"
upd:"Always"
"Always"
"Завжди"

ConfigCmdlineUseShell
"Использовать &шелл"
"Use &shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
"Використовувати &шелл"

ConfigCmdlineUsePromptFormat
"Установить &формат командной строки"
"Set command line &prompt format"
"Nastavit formát &příkazového řádku"
"&Promptformat der Kommandozeile"
"Parancssori &prompt formátuma"
"Wy&gląd znaku zachęty linii poleceń"
"Formato para línea de comando (&prompt)"
"Встановити &формат командного рядка"

ConfigCmdlinePromptFormatAdmin
"Администратор"
"Administrator"
upd:"Administrator"
upd:"Administrator"
upd:"Administrator"
upd:"Administrator"
"Administrador"
"Адміністратор"

ConfigAutoCompleteTitle
l:
"Настройка автозавершения"
"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
"Opciones de autocompletar"
"Налаштування автозавершення"

ConfigAutoCompleteExceptions
l:
"Шаблоны &исключений"
"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
"Шаблони &винятковий"

ConfigAutoCompleteShowList
l:
"Показывать &список"
"&Show list"
upd:"&Show list"
upd:"&Show list"
upd:"&Show list"
upd:"&Show list"
"Mo&strar lista"
"Показувати &список"

ConfigAutoCompleteModalList
l:
"&Модальный режим"
"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
"Clase de &Modo"
"&Модальний режим"

ConfigAutoCompleteAutoAppend
l:
"&Подставлять первый подходящий вариант"
"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
"&Agregar primer ítem coincidente"
"&підставляти перший відповідний варіант"

ConfigInfoPanelTitle
l:
"Настройка информационной панели"
"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
"Opciones de panel de información"
"налаштування інформаційної панелі"

ViewConfigTitle
l:
"Программа просмотра"
"Viewer"
"Prohlížeč"
"Betrachter"
"Nézőke"
"Podgląd"
"Visor"
"Програма перегляду"

ViewConfigExternalF3
"Запускать внешнюю программу просмотра по F3 вместо Alt-F3"
"Use external viewer for F3 instead of Alt-F3"
upd:"Use external viewer for F3 instead of Alt-F3"
upd:"Use external viewer for F3 instead of Alt-F3"
"Alt-F3 helyett F3 indítja a külső nézőkét"
upd:"Use external viewer for F3 instead of Alt-F3"
"Usar visor externo con F3 en lugar de Alt-F3"
"запускати зовнішню програму перегляду F3 замість Alt-F3"

ViewConfigExternalCommand
"&Команда просмотра:"
"&Viewer command:"
"&Příkaz prohlížeče:"
"Befehl für e&xternen Betracher:"
"Nézőke &parancs:"
"&Polecenie:"
"Comando &visor:"
"&Команда перегляду:"

ViewConfigInternal
" Встроенная программа просмотра "
" Internal viewer "
" Interní prohlížeč "
" Interner Betracher "
" Belső nézőke "
" Podgląd wbudowany "
"Visor interno"
" Вбудована программа перегляду "

ViewConfigSavePos
"&Сохранять позицию файла"
"&Save file position"
"&Ukládat pozici v souboru"
"Dateipositionen &speichern"
"&Fájlpozíció mentése"
"&Zapamiętaj pozycję w pliku"
"&Guardar posición de archivo"
"&Зберігати позицію файлу"

ViewConfigSaveShortPos
"Сохранять &закладки"
"Save &bookmarks"
"Ukládat &záložky"
"&Lesezeichen speichern"
"Könyv&jelzők mentése"
"Zapisz z&akładki"
"Guardar &marcadores"
"Зберігати &закладки"

ViewAutoDetectCodePage
"&Автоопределение кодовой страницы"
"&Autodetect code page"
upd:"&Autodetekovat znakovou sadu"
upd:"Zeichentabelle &automatisch erkennen"
"&Kódlap automatikus felismerése"
"Rozpozn&aj tablicę znaków"
"&Autodetectar tabla de caracteres"
"&Автовизначення кодової сторінки"

ViewConfigTabSize
"Раз&мер табуляции"
"Tab si&ze"
"Velikost &Tabulátoru"
"Ta&bulatorgröße"
"Ta&bulátor mérete"
"Rozmiar &tabulatora"
"Tamaño de &tabulación"
"Роз&мір табуляції"

ViewConfigScrollbar
"Показывать &полосу прокрутки"
"Show scro&llbar"
"Zobrazovat posu&vník"
"Scro&llbalken anzeigen"
"Gör&dítősáv mutatva"
"Pokaż &pasek przewijania"
"Mostrar barra de desp&lazamiento"
"Показувати &полосу прокрутки"

ViewConfigArrows
"Показывать стрелки с&двига"
"Show scrolling arro&ws"
"Zobrazovat &skrolovací šipky"
"P&feile bei Scrollbalken zeigen"
"Gördítőn&yilak mutatva"
"Pokaż strzał&ki przewijania"
"Mostrar flechas de despla&zamiento"
"Показувати стрілки з&суву"

ViewShowKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавиш"

ViewShowTitleBar
"Показывать &заголовок"
"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
"Показувати &заголовок"

ViewConfigPersistentSelection
"Постоянное &выделение"
"&Persistent selection"
"Trvalé &výběry"
"Dauerhafte Text&markierungen"
"&Maradó blokkok"
"T&rwałe zaznaczenie"
"Selección &persistente"
"Постійне &виділення"

ViewConfigDefaultCodePage
"Выберите &кодовую страницу по умолчанию:"
"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
"Виберіть &кодову сторінку за замовчуванням:"

EditConfigTitle
l:
"Редактор"
"Editor"
"Editor"
"Editor"
"Szerkesztő"
"Edytor"
"Editor"
"Редактор"

EditConfigEditorF4
"Запускать внешний редактор по F4 вместо Alt-F4"
"Use external editor for F4 instead of Alt-F4"
upd:"Use external editor for F4 instead of Alt-F4"
upd:"Use external editor for F4 instead of Alt-F4"
"Alt-F4 helyett F4 indítja a külső szerkesztőt"
upd:"Use external editor for F4 instead of Alt-F4"
"Usar editor externo con F4 en lugar de Alt-F4"
"Запускати зовнішній редактор F4 замість Alt-F4"

EditConfigEditorCommand
"&Команда редактирования:"
"&Editor command:"
"&Příkaz editoru:"
"Befehl für e&xternen Editor:"
"&Szerkesztő parancs:"
"&Polecenie:"
"Comando &editor:"
"&Команда редагування:"

EditConfigInternal
" Встроенный редактор "
" Internal editor "
" Interní editor "
" Interner Editor "
" Belső szerkesztő "
" Edytor wbudowany "
"Editor interno"
" Вбудований редактор "

EditConfigExpandTabsTitle
"Преобразовывать &табуляцию:"
"Expand &tabs:"
"Rozšířit Ta&bulátory mezerami"
"&Tabs expandieren:"
"&Tabulátorból szóközök:"
"Zamiana znaków &tabulacji:"
"Expandir &tabulación a espacios"
"Перетворити &табуляцію:"

EditConfigDoNotExpandTabs
l:
"Не преобразовывать табуляцию"
"Do not expand tabs"
"Nerozšiřovat tabulátory mezerami"
"Tabs nicht expandieren"
"Ne helyettesítse a tabulátorokat"
"Nie zamieniaj znaków tabulacji"
"No expandir tabulacines"
"Не перетворювати табуляцію"

EditConfigExpandTabs
"Преобразовывать новые символы табуляции в пробелы"
"Expand newly entered tabs to spaces"
"Rozšířit nově zadané tabulátory mezerami"
"Neue Tabs zu Leerzeichen expandieren"
"Újonnan beírt tabulátorból szóközök"
"Zamień nowo dodane znaki tabulacji na spacje"
"Expandir nuevas tabulaciones ingresadas a espacios"
"Перетворити нові символи табуляції на пробіли"

EditConfigConvertAllTabsToSpaces
"Преобразовывать все символы табуляции в пробелы"
"Expand all tabs to spaces"
"Rozšířit všechny tabulátory mezerami"
"Alle Tabs zu Leerzeichen expandieren"
"Minden tabulátorból szóközök"
"Zastąp wszystkie tabulatory spacjami"
"Expandir todas las tabulaciones a espacios"
"Перетворити всі символи табуляції на пробіли"

EditConfigPersistentBlocks
"&Постоянные блоки"
"&Persistent blocks"
"&Trvalé bloky"
"Dauerhafte Text&markierungen"
"&Maradó blokkok"
"T&rwałe bloki"
"Bloques &persistente"
"&Постійні блоки"

EditConfigDelRemovesBlocks
l:
"Del удаляет б&локи"
"&Del removes blocks"
"&Del maže bloky"
"&Entf löscht Textmark."
"A &Del törli a blokkokat"
"&Del usuwa bloki"
"Del &remueve bloques"
"Del видаляє б&локи"

EditConfigAutoIndent
"Авто&отступ"
"Auto &indent"
"Auto &Odsazování"
"Automatischer E&inzug"
"Automatikus &behúzás"
"Automatyczne &wcięcia"
"Auto &dentar"
"Авто&відступ"

EditConfigSavePos
"&Сохранять позицию файла"
"&Save file position"
"&Ukládat pozici v souboru"
"Dateipositionen &speichern"
"Fájl&pozíció mentése"
"&Zapamiętaj pozycję kursora w pliku"
"&Guardar posición de archivo"
"&Зберігати позицію файлу"

EditConfigSaveShortPos
"Сохранять &закладки"
"Save &bookmarks"
"Ukládat zá&ložky"
"&Lesezeichen speichern"
"Könyv&jelzők mentése"
"Zapisz &zakładki"
"Guardar &marcadores"
"Зберігати &закладки"

EditCursorBeyondEnd
"Ку&рсор за пределами строки"
"&Cursor beyond end of line"
"&Kurzor za koncem řádku"
upd:"&Cursor hinter dem Ende"
"Kurzor a sor&végjel után is"
"&Kursor za końcem linii"
"&Cursor después de fin de línea"
"Ку&рсор за межами рядка"

EditAutoDetectCodePage
"&Автоопределение кодовой страницы"
"&Autodetect code page"
upd:"&Autodetekovat znakovou sadu"
upd:"Zeichentabelle &automatisch erkennen"
"&Kódlap automatikus felismerése"
"Rozpozn&aj tablicę znaków"
"&Autodetectar tabla de caracteres"
"&Автовизначення кодової сторінки"

EditShareWrite
"Разрешить редактирование открытых для записи &файлов"
"Allow editing files ope&ned for writing"
upd:"Allow editing files opened for &writing"
upd:"Allow editing files opened for &writing"
"Írásra m&egnyitott fájlok szerkeszthetők"
upd:"Allow editing files opened for &writing"
"Permitir escritura de archivos abiertos para edición"
"Дозволити редагування відкритих для запису &файлів"

EditLockROFileModification
"Блокировать р&едактирование файлов с атрибутом R/O"
"Lock editing of read-only &files"
"&Zamknout editaci souborů určených jen pro čtení"
"Bearbeiten von &Dateien mit Schreibschutz verhindern"
"Csak olvasható fájlok s&zerkesztése tiltva"
"Nie edytuj plików tylko do odczytu"
"Bloquear edición de &archivos de sólo lectura"
"Блокувати р&едагування файлів з атрибутом R/O"

EditWarningBeforeOpenROFile
"Пре&дупреждать при открытии файла с атрибутом R/O"
"&Warn when opening read-only files"
"&Varovat při otevření souborů určených jen pro čtení"
"Beim Öffnen von Dateien mit Schreibschutz &warnen"
"Figyelmeztet &csak olvasható fájl megnyitásakor"
"&Ostrzeż przed otwieraniem plików tylko do odczytu"
"Advertencia al abrir archivos de sólo lectura"
"Зап&обігати відкриванню файлу з атрибутом R/O"

EditConfigTabSize
"Раз&мер табуляции"
"Tab si&ze"
"Velikost &Tabulátoru"
"Ta&bulatorgröße"
"Tab&ulátor mérete"
"Rozmiar ta&bulatora"
"Tamaño de tabulación"
"Роз&мір табуляції"

EditConfigScrollbar
"Показывать &полосу прокрутки"
"Show scro&llbar"
"Zobr&azovat posuvník"
"Scro&llbalken anzeigen"
"&Gördítősáv mutatva"
"Pokaż %pasek przewijania"
"Mostrar barra de desp&lazamiento"
"Показувати &смугу прокручування"

EditShowWhiteSpace
"Пробельные символы"
"Show white space"
upd:"Show white space"
upd:"Show white space"
upd:"Show white space"
upd:"Show white space"
"Mostrar espacios en blanco"
"Пробільні символи"

EditShowKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавіш"

EditShowTitleBar
"Показывать &заголовок"
"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
"Показувати &заголовок"

EditConfigPickUpWord
"Cлово под к&урсором"
"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
"Pick &up the word"
"Слово під к&урсором"

EditConfigDefaultCodePage
"Выберите &кодовую страницу по умолчанию:"
"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
"Виберіть &кодову сторінку за промовчанням:"

NotifConfigTitle
l:
"Уведомления"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Повідомлення"

NotifConfigOnFileOperation
"Уведомлять о завершении &файловой операции"
"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
"Повідомляти про завершення &файлової операції"

NotifConfigOnConsole
"Уведомлять о завершении &консольной команды"
"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
"Повідомляти про завершення &консольної команди"

NotifConfigOnlyIfBackground
"Уведомлять только когда в &фоне"
"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
"Повідомляти лише коли у &фоні"

ConsoleCommandComplete
"Консольная команда выполнена"
"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
"Консольна команда виконана"

ConsoleCommandFailed
"Консольная команда завершена с ошибкой"
"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
"Консольна команда завершена з помилкою"

FileOperationComplete
"Файловая операция выполнена"
"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
"Файлова операція виконана"

SaveSetupTitle
l:
"Конфигурация"
"Save setup"
"Uložit nastavení"
"Einstellungen speichern"
"Beállítások mentése"
"Zapisz ustawienia"
"Guardar configuración"
"Конфігурація"

SaveSetupAsk1
"Вы хотите сохранить"
"Do you wish to save"
"Přejete si uložit"
"Wollen Sie die aktuellen Einstellungen"
"Elmenti a jelenlegi"
"Czy chcesz zapisać"
"Desea guardar la configuración"
"Ви хочете зберегти"

SaveSetupAsk2
"текущую конфигурацию?"
"current setup?"
"aktuální nastavení?"
"speichern?"
"beállításokat?"
"bieżące ustawienia?"
"actual de FAR?"
"Поточну конфігурацію?"

SaveSetup
"Сохранить"
"Save"
"Uložit"
"Speichern"
"Mentés"
"Zapisz"
"Guardar"
"Зберегти"

CopyDlgTitle
l:
"Копирование"
"Copy"
"Kopírovat"
"Kopieren"
"Másolás"
"Kopiuj"
"Copiar"
"Копіювання"

MoveDlgTitle
"Переименование/Перенос"
"Rename/Move"
"Přejmenovat/Přesunout"
"Verschieben/Umbenennen"
"Átnevezés-Mozgatás"
"Zmień nazwę/przenieś"
"Renombrar/Mover"
"Перейменування/Перенесення"

LinkDlgTitle
"Ссылка"
"Link"
"Link"
"Link erstellen"
"Link létrehozása"
"Dowiąż"
"Enlace"
"Посилання"

CopyAccessMode
"Копировать &режим доступа к файлам"
"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
"Копіювати &режим доступу до файлів"

CopyIfFileExist
"Уже су&ществующие файлы:"
"Already e&xisting files:"
"Již e&xistující soubory:"
"&Dateien überschreiben:"
"Már &létező fájloknál:"
"Dla już &istniejących:"
"Archivos ya e&xistentes:"
"Вже іс&нуючі файли:"

CopyAsk
"&Запрос действия"
"&Ask"
"Ptát s&e"
"Fr&agen"
"Kér&dez"
"&Zapytaj"
"Pregunt&ar"
"&Запит дії"

CopyAskRO
"Запрос подтверждения для &R/O файлов"
"Also ask on &R/O files"
"Ptát se také na &R/O soubory"
"Bei Dateien mit Sch&reibschutz fragen"
"&Csak olvasható fájloknál is kérdez"
"&Pytaj także o pliki tylko do odczytu"
"Preguntar también en archivos de Sólo Lectu&ra"
"Запит підтвердження для &R/O файлів"

CopyOnlyNewerFiles
"Только &новые/обновлённые файлы"
"Only ne&wer file(s)"
"Pouze &novější soubory"
"Nur &neuere Dateien"
"Cs&ak az újabb fájlokat"
"Tylko &nowsze pliki"
"Sólo archivo(s) más nuev&os"
"Тільки &нові/оновлені файли"

LinkType
"&Тип ссылки:"
"Link t&ype:"
"&Typ linku:"
"Linkt&yp:"
"Link &típusa:"
"&Typ linku:"
"Tipo de &enlace"
"&Тип посилання:"

LinkTypeHardlink
"&жёсткая ссылка"
"&hard link"
"&pevný link"
"&Hardlink"
"&Hardlink"
"link &trwały"
"enlace duro"
"&жорстке посилання"

LinkTypeSymlink
"си&мволическая ссылка"
"&symbolic link"
"symbolický link"
"Symbolischer Link"
"Szimbolikus link"
"link symboliczny"
"enlace simbólico"
"си&мволічне посилання"

CopySymLinkText
"Символические сс&ылки:"
"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
"Символічні по&силання:"

LinkCopyAsIs
"Всегда копировать &ссылку"
"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
"Завжди копіювати &посилання"

LinkCopySmart
"&Умно копировать ссылку или файл"
"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
"&Розумно копіювати посилання або файл"

LinkCopyContent
"Копировать как &файл"
"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
"Копіювати як &файл"

CopySparseFiles
"Создавать &разреженные файлы"
"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
"Створювати &розріджені файли"

CopyUseCOW
"Использовать копирование-&при-записи если возможно"
"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
"Використовувати копіювання-&та-записи якщо це можливо"

CopyMultiActions
"Обр&абатывать несколько имён файлов"
"Process &multiple destinations"
"&Zpracovat více míst určení"
"&Mehrere Ziele verarbeiten"
"Tö&bbszörös cél létrehozása"
"Przetwarzaj &wszystkie cele"
"Procesar &múltiples destinos"
"Виб&рати кілька імен файлів"

CopyDlgCopy
"&Копировать"
"&Copy"
"&Kopírovat"
"&Kopieren"
"&Másolás"
"&Kopiuj"
"&Copiar"
"&Копіювати"

CopyDlgTree
"F10-&Дерево"
"F10-&Tree"
"F10-&Strom"
"F10-&Baum"
"F10-&Fa"
"F10-&Drzewo"
"F10-&Arbol"
"F10-&Дерево"

CopyDlgCancel
"&Отменить"
"Ca&ncel"
"&Storno"
"Ab&bruch"
"Még&sem"
"&Anuluj"
"Ca&ncelar"
"&Скасувати"

CopyDlgRename
"&Переименовать"
"&Rename"
"Přej&menovat"
"&Umbenennen"
"Át&nevez-Mozgat"
"&Zmień nazwę"
"&Renombrar"
"&Перейменувати"

CopyDlgLink
"&Создать ссылку"
"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
"&Створити посилання"

CopyDlgTotal
"Всего"
"Total"
"Celkem"
"Gesamt"
"Összesen"
"Razem"
"Total"
"Всього"

CopyScanning
"Сканирование папок..."
"Scanning folders..."
"Načítání adresářů..."
"Scanne Ordner..."
"Mappák olvasása..."
"Przeszukuję katalogi..."
"Explorando directorios..."
"Сканування тек..."

CopyPrepareSecury
"Применение прав доступа..."
"Applying access rights..."
"Nastavuji přístupová práva..."
"Anwenden der Zugriffsrechte..."
"Hozzáférési jogok alkalmazása..."
"Ustawianie praw dostępu..."
"Aplicando derechos de acceso..."
"Застосування прав доступу..."

CopyUseFilter
"Исполь&зовать фильтр"
"&Use filter"
"P&oužít filtr"
"Ben&utze Filter"
"Szűrő&vel"
"&Użyj filtra"
"&Usar filtros"
"Викори&стовувати фільтр"

CopySetFilter
"&Фильтр"
"Filt&er"
"Filt&r"
"Filt&er"
"S&zűrő"
"Filt&r"
"Fi&ltro"
"&Фільтр"

CopyFile
l:
"Копировать"
"Copy"
"Kopírovat"
"Kopiere"
upd:"másolása"
"Skopiuj"
"Copiar"
"Копіювати"

MoveFile
"Переименовать или перенести"
"Rename or move"
"Přejmenovat nebo přesunout"
"Verschiebe"
upd:"átnevezése-mozgatása"
"Zmień nazwę lub przenieś"
"Renombrar o mover"
"Перейменувати або перенести"

LinkFile
"Создать ссылку на"
"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
"Створити посилання на"

CopyFiles
"Копировать %d элемент%ls"
"Copy %d item%ls"
"Kopírovat %d polož%ls"
"Kopiere %d Objekt%ls"
" %d elem másolása"
"Skopiuj %d plików"
"Copiar %d ítem%ls"
"Копіювати %d елемент%ls"

MoveFiles
"Переименовать или перенести %d элемент%ls"
"Rename or move %d item%ls"
"Přejmenovat nebo přesunout %d polož%ls"
"Verschiebe %d Objekt%ls"
" %d elem átnevezése-mozgatása"
"Zmień nazwę lub przenieś %d plików"
"Renombrar o mover %d ítem%ls"
"Перейменувати або перенести %d елемент %ls"

LinkFiles
"Создать ссылки на %d элемент%ls"
"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
"Створити посилання на %d елемент %ls"

CMLTargetTO
" &в:"
" t&o:"
" d&o:"
" na&ch:"
" ide:"
" d&o:"
" &hacia:"
" &в:"

CMLTargetIN
" &в:"
" in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
" &в:"

CMLItems0
""
""
"u"
""
""
""
""
""

CMLItemsA
"а"
"s"
"ek"
"e"
""
"s"
"s"
"а"

CMLItemsS
"ов"
"s"
"ky"
"e"
""
"s"
"s"
"ів"

CopyIncorrectTargetList
l:
"Указан некорректный список целей"
"Incorrect target list"
"Nesprávný seznam cílů"
"Ungültige Liste von Zielen"
"Érvénytelen céllista"
"Błędna lista wynikowa"
"Lista destino incorrecta"
"Вказано некоректний список цілей"

CopyCopyingTitle
l:
"Копирование"
"Copying"
"Kopíruji"
"Kopieren"
"Másolás"
"Kopiowanie"
"Copiando"
"Копіювання"

CopyMovingTitle
"Перенос"
"Moving"
"Přesouvám"
"Verschieben"
"Mozgatás"
"Przenoszenie"
"Moviendo"
"Перенесення"

CopyCannotFind
l:
"Файл не найден"
"Cannot find the file"
"Nelze nalézt soubor"
"Folgende Datei kann nicht gefunden werden:"
"A fájl nem található:"
"Nie mogę odnaleźć pliku"
"No se puede encontrar el archivo"
"Файл не знайдено"

CannotCopyFolderToItself1
l:
"Нельзя копировать папку"
"Cannot copy the folder"
"Nelze kopírovat adresář"
"Folgender Ordner kann nicht kopiert werden:"
"A mappa:"
"Nie można skopiować katalogu"
"No se puede copiar el directorio"
"Не можна копіювати папку"

CannotCopyFolderToItself2
"в саму себя"
"onto itself"
"sám na sebe"
"Ziel und Quelle identisch."
"nem másolható önmagába/önmagára"
"do niego samego"
"en sí mismo"
"у саму себе"

CannotCopyToTwoDot
l:
"Нельзя копировать файл или папку"
"You may not copy files or folders"
"Nelze kopírovat soubory nebo adresáře"
"Kopieren von Dateien oder Ordnern ist maximal"
"Nem másolhatja a fájlt vagy mappát"
"Nie można skopiować plików"
"Usted no puede copiar archivos o directorios"
"Не можна копіювати файл або папку"

CannotMoveToTwoDot
"Нельзя перемещать файл или папку"
"You may not move files or folders"
"Nelze přesunout soubory nebo adresáře"
"Verschieben von Dateien oder Ordnern ist maximal"
"Nem mozgathatja a fájlt vagy mappát"
"Nie można przenieść plików"
"Usted no puede mover archivos o directorios"
"Не можна переміщувати файл або папку"

CannotCopyMoveToTwoDot
"выше корневого каталога"
"higher than the root folder"
"na vyšší úroveň než kořenový adresář"
"bis zum Wurzelverzeichnis möglich."
"a gyökér fölé"
"na poziom wyższy niż do korzenia"
"más alto que el directorio raíz"
"вище кореневого каталогу"

CopyCannotCreateFolder
l:
"Ошибка создания папки"
"Cannot create the folder"
"Nelze vytvořit adresář"
"Folgender Ordner kann nicht erstellt werden:"
"A mappa nem hozható létre:"
"Nie udało się utworzyć katalogu"
"No se puede crear el directorio"
"Помилка створення папки"

CopyCannotRenameFolder
"Невозможно переименовать папку"
"Cannot rename the folder"
"Nelze přejmenovat adresář"
"Folgender Ordner kann nicht umbenannt werden:"
"A mappa nem nevezhető át:"
"Nie udało się zmienić nazwy katalogu"
"No se puede renombrar el directorio"
"Неможливо перейменувати папку"

CopyIgnore
"&Игнорировать"
"&Ignore"
"&Ignorovat"
"&Ignorieren"
"Mé&gis"
"&Ignoruj"
"&Ignorar"
"&Ігнорувати"

CopyRetry
"&Повторить"
"&Retry"
"&Opakovat"
"Wiede&rholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

CopySkip
"П&ропустить"
"&Skip"
"&Přeskočit"
"Ausla&ssen"
"&Kihagy"
"&Omiń"
"&Omitir"
"П&ропустити"

CopySkipAll
"&Пропустить все"
"S&kip all"
"Př&eskočit vše"
"Alle aus&lassen"
"Mi&nd"
"Omiń w&szystkie"
"O&mitir todos"
"&Пропустити все"

CopyCancel
"&Отменить"
"&Cancel"
"&Storno"
"Abb&rechen"
"Még&sem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

CopyCannotCreateJunctionToFile
"Невозможно создать связь. Файл уже существует:"
"Cannot create junction. The file already exists:"
"Nelze vytvořit křížový odkaz. Soubor již existuje:"
"Knotenpunkt wurde nicht erstellt. Datei existiert bereits:"
"A csomópont nem hozható létre. A fájl már létezik:"
"Nie można utworzyć połączenia - plik już istnieje:"
"No se puede unir. El archivo ya existe:"
"Неможливо створити зв'язок. Файл уже існує:"

CopyCannotCreateSymlinkAskCopyContents
"Невозможно создать связь. Копировать данные вместо связей?"
"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
"Неможливо створити зв'язок. Копіювати дані замість зв'язків?"

CannotCopyFileToItself1
l:
"Нельзя копировать файл"
"Cannot copy the file"
"Nelze kopírovat soubor"
"Folgende Datei kann nicht kopiert werden:"
"A fájl"
"Nie można skopiować pliku"
"Imposible copiar el archivo"
"Не можна копіювати файл"

CannotCopyFileToItself2
"в самого себя"
"onto itself"
"sám na sebe"
"Ziel und Quelle identisch."
"nem másolható önmagára"
"do niego samego"
"en sí mismo"
"у самого себе"

CopyDirectoryOrFile
l:
"Подразумевается имя папки или файла?"
"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
"Si especifica nombre de carpeta o nombre de archivo?"
"Має на увазі ім'я теки або файлу?"

CopyDirectoryOrFileDirectory
"Папка"
"Folder"
upd:"Folder"
upd:"Folder"
upd:"Folder"
upd:"Folder"
"Carpeta"
"Тека"

CopyDirectoryOrFileFile
"Файл"
"File"
upd:"File"
upd:"File"
upd:"File"
upd:"File"
"Archivo"
"Файл"

CopyFileExist
l:
"Файл уже существует"
"File already exists"
"Soubor již existuje"
"Datei existiert bereits"
"A fájl már létezik:"
"Plik już istnieje"
"El archivo ya existe"
"Файл вже існує"

CopySource
"&Новый"
"&New"
"&Nový"
"&Neue Datei"
"Ú&j verzió:"
"&Nowy"
"Nuevo"
"&Новий"

CopyDest
"Су&ществующий"
"E&xisting"
"E&xistující"
"Be&stehende Datei"
"Létező &verzió:"
"&Istniejący"
"Existente"
"Іс&нуючий"

CopyOverwrite
"В&место"
"&Overwrite"
"&Přepsat"
"Über&schr."
"&Felülír"
"N&adpisz"
"&Sobrescribir"
"З&амість"

CopySkipOvr
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&spr."
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

CopyAppend
"&Дописать"
"A&ppend"
"Př&ipojit"
"&Anhängen"
"Hoz&záfűz"
"&Dołącz"
"A&gregar"
"&Дописати"

CopyRename
"&Имя"
"R&ename"
upd:"R&ename"
upd:"R&ename"
"Á&tnevez"
upd:"R&ename"
"Renombrar"
"&Ім'я"

CopyCancelOvr
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Відмінити"

CopyRememberChoice
"&Запомнить выбор"
"&Remember choice"
"Zapama&tovat volbu"
"Auswahl me&rken"
"Mind&ent a kiválasztott módon"
"&Zapamiętaj ustawienia"
"&Recordar elección"
"&Запам'ятати вибір"

CopyRenameTitle
"Переименование"
"Rename"
upd:"Rename"
upd:"Rename"
"Átnevezés"
upd:"Rename"
"Renombrar"
"Перейменування"

CopyRenameText
"&Новое имя:"
"&New name:"
upd:"&New name:"
upd:"&New name:"
"Ú&j név:"
upd:"&New name:"
"&Nuevo nombre:"
"&Нове ім'я:"

CopyFileRO
l:
"Файл имеет атрибут \"Только для чтения\""
"The file is read only"
"Soubor je určen pouze pro čtení"
"Folgende Datei ist schreibgeschützt:"
"A fájl csak olvasható:"
"Ten plik jest tylko-do-odczytu"
"El archivo es de sólo lectura"
"Файл має атрибут \"Тільки для читання\""

CopyAskDelete
"Вы хотите удалить его?"
"Do you wish to delete it?"
"Opravdu si ho přejete smazat?"
"Wollen Sie sie dennoch löschen?"
"Biztosan törölni akarja?"
"Czy chcesz go usunąć?"
"Desea borrarlo igual?"
"Ви хочете видалити його?"

CopyDeleteRO
"&Удалить"
"&Delete"
"S&mazat"
"&Löschen"
"&Törli"
"&Usuń"
"&Borrar"
"&Вилучити"

CopyDeleteAllRO
"&Все"
"&All"
"&Vše"
"&Alle Löschen"
"Min&det"
"&Wszystkie"
"&Todos"
"&Усе"

CopySkipRO
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagyja"
"&Omiń"
"&Omitir"
"&Пропустити"

CopySkipAllRO
"П&ропустить все"
"S&kip all"
"Př&eskočit vše"
"A&lle überspringen"
"Mind&et"
"O&miń wszystkie"
"O&mitir todos"
"П&ропустити все"

CopyCancelRO
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

CannotCopy
l:
"Ошибка копирования"
"Cannot copy"
"Nelze kopírovat"
"Konnte nicht kopieren"
"Nem másolható"
"Nie mogę skopiować"
"No se puede copiar %ls"
"Помилка копіювання"

CannotMove
"Ошибка переноса"
"Cannot move"
"Nelze přesunout"
"Konnte nicht verschieben"
"Nem mozgatható"
"Nie mogę przenieść"
"No se puede mover %ls"
"Помилка перенесення"

CannotLink
"Ошибка создания ссылки"
"Cannot link"
"Nelze linkovat"
"Konnte nicht verlinken"
"Nem linkelhető"
"Nie mogę dowiązać"
"No se puede enlazar %ls"
"Помилка створення посилання"

CannotCopyTo
"в"
"to"
"do"
"nach"
"ide:"
"do"
"hacia %ls"
"в"

CopyReadError
l:
"Ошибка чтения данных из"
"Cannot read data from"
"Nelze číst data z"
"Kann Daten nicht lesen von"
"Nem olvasható adat innen:"
"Nie mogę odczytać danych z"
"No se puede leer datos desde"
"Помилка читання даних з"

CopyWriteError
"Ошибка записи данных в"
"Cannot write data to"
"Nelze zapsat data do"
"Dann Daten nicht schreiben in"
"Nem írható adat ide:"
"Nie mogę zapisać danych do"
"No se puede escribir datos hacia"
"Помилка запису даних"

CopyProcessed
l:
"Обработано файлов: %d"
"Files processed: %d"
"Zpracováno souborů: %d"
"Dateien verarbeitet: %d"
" %d fájl kész"
"Przetworzonych plików: %d"
"Archivos procesados: %d"
"Оброблено файли: %d"

CopyProcessedTotal
"Обработано файлов: %d из %d"
"Files processed: %d of %d"
"Zpracováno souborů: %d z %d"
"Dateien verarbeitet: %d von %d"
" %d fájl kész %d fájlból"
"Przetworzonych plików: %d z %d"
"Archivos procesados: %d de %d"
"Оброблено файли: %d з %d"

CopyMoving
"Перенос файла"
"Moving the file"
"Přesunuji soubor"
"Verschiebe die Datei"
"Fájl mozgatása"
"Przenoszę plik"
"Moviendo el archivo"
"Перенесення файлу"

CopyCopying
"Копирование файла"
"Copying the file"
"Kopíruji soubor"
"Kopiere die Datei"
"Fájl másolása"
"Kopiuję plik"
"Copiando el archivo"
"Копіювання файлу"

CopyTo
"в"
"to"
"do"
"nach"
"ide:"
"do"
"Hacia"
"в"

DeleteTitle
l:
"Удаление"
"Delete"
"Smazat"
"Löschen"
"Törlés"
"Usuń"
"Borrar"
"Видалення"

AskDeleteFolder
"Вы хотите удалить папку"
"Do you wish to delete the folder"
"Přejete si smazat adresář"
"Wollen Sie den Ordner löschen"
"Törölni akarja a mappát?"
"Czy chcesz wymazać katalog"
"Desea borrar el directorio"
"Ви хочете видалити теку"

AskDeleteFile
"Вы хотите удалить файл"
"Do you wish to delete the file"
"Přejete si smazat soubor"
"Wollen Sie die Datei löschen"
"Törölni akarja a fájlt?"
"Czy chcesz usunąć plik"
"Desea borrar el archivo"
"Ви хочете видалити файл"

AskDelete
"Вы хотите удалить"
"Do you wish to delete"
"Přejete si smazat"
"Wollen Sie folgendes Objekt löschen"
"Törölni akar"
"Czy chcesz usunąć"
"Desea borrar"
"Ви хочете видалити"

AskDeleteRecycleFolder
"Вы хотите переместить в Корзину папку"
"Do you wish to move to the Recycle Bin the folder"
"Přejete si přesunout do Koše adresář"
"Wollen Sie den Ordner in den Papierkorb verschieben"
"A Lomtárba akarja dobni a mappát?"
"Czy chcesz przenieść katalog do Kosza"
"Desea mover hacia la papelera de reciclaje el directorio"
"Ви хочете перемістити в Кошик теку"

AskDeleteRecycleFile
"Вы хотите переместить в Корзину файл"
"Do you wish to move to the Recycle Bin the file"
"Přejete si přesunout do Koše soubor"
"Wollen Sie die Datei in den Papierkorb verschieben"
"A Lomtárba akarja dobni a fájlt?"
"Czy chcesz przenieść plik do Kosza"
"Desea mover hacia la papelera de reciclaje el archivo"
"Ви хочете перемістити в Кошик файл"

AskDeleteRecycle
"Вы хотите переместить в Корзину"
"Do you wish to move to the Recycle Bin"
"Přejete si přesunout do Koše"
"Wollen Sie das Objekt in den Papierkorb verschieben"
"A Lomtárba akar dobni"
"Czy chcesz przenieść do Kosza"
"Desea mover hacia la papelera de reciclaje"
"Ви хочете перемістити в Кошик"

DeleteWipeTitle
"Уничтожение"
"Wipe"
"Vymazat"
"Sicheres Löschen"
"Kisöprés"
"Wymaż"
"Limpiar"
"Знищення"

AskWipeFolder
"Вы хотите уничтожить папку"
"Do you wish to wipe the folder"
"Přejete si vymazat adresář"
"Wollen Sie den Ordner sicher löschen"
"Ki akarja söpörni a mappát?"
"Czy chcesz wymazać katalog"
"Desea limpiar el directorio"
"Ви хочете знищити теку"

AskWipeFile
"Вы хотите уничтожить файл"
"Do you wish to wipe the file"
"Přejete si vymazat soubor"
"Wollen Sie die Datei sicher löschen"
"Ki akarja söpörni a fájlt?"
"Czy chcesz wymazać plik"
"Desea limpiar el archivo"
"Ви хочете знищити файл"

AskWipe
"Вы хотите уничтожить"
"Do you wish to wipe"
"Přejete si vymazat"
"Wollen Sie das Objekt sicher löschen"
"Ki akar söpörni"
"Czy chcesz wymazać"
"Desea limpiar"
"Ви хочете знищити"

AskDeleteItems
"%d элемент%ls"
"%d item%ls"
"%d polož%ls"
"%d Objekt%ls"
"%d elemet%ls"
"%d plik%ls"
"%d ítem%ls"
"%d елемент%ls"

AskDeleteItems0
""
""
"ku"
""
""
""
""
""

AskDeleteItemsA
"а"
"s"
"ky"
"e"
""
"i"
"s"
"а"

AskDeleteItemsS
"ов"
"s"
"ek"
"e"
""
"ów"
"s"
"ів"

DeleteFolderTitle
l:
"Удаление папки "
"Delete folder"
"Smazat adresář"
"Ordner löschen"
"Mappa törlése"
"Usuń folder"
"Borrar directorio"
"Видалення теки "

WipeFolderTitle
"Уничтожение папки "
"Wipe folder"
"Vymazat adresář"
"Ordner sicher löschen"
"Mappa kisöprése"
"Wymaż folder"
"Limpiar directorio"
"Знищення теки "

DeleteFilesTitle
"Удаление файлов"
"Delete files"
"Smazat soubory"
"Dateien löschen"
"Fájlok törlése"
"Usuń pliki"
"Borrar archivos"
"Видалення файлів"

WipeFilesTitle
"Уничтожение файлов"
"Wipe files"
"Vymazat soubory"
"Dateien sicher löschen"
"Fájlok kisöprése"
"Wymaż pliki"
"Limpiar archivos"
"Знищення файлів"

DeleteFolderConfirm
"Данная папка будет удалена:"
"The following folder will be deleted:"
"Následující adresář bude smazán:"
"Folgender Ordner wird gelöscht:"
"A mappa törlődik:"
"Następujący folder zostanie usunięty:"
"El siguiente directorio será borrado:"
"Ця тека буде видалена:"

WipeFolderConfirm
"Данная папка будет уничтожена:"
"The following folder will be wiped:"
"Následující adresář bude vymazán:"
"Folgender Ordner wird sicher gelöscht:"
"A mappa kisöprődik:"
"Następujący folder zostanie wymazany:"
"El siguiente directorio será limpiado:"
"Ця тека буде знищена:"

DeleteWipe
"Уничтожить"
"Wipe"
"Vymazat"
"Sicheres Löschen"
"Kisöpör"
"Wymaż"
"Limpiar"
"Знищити"

DeleteRecycle
"Переместить"
"Move"
upd:"Move"
upd:"Move"
upd:"Move"
upd:"Move"
upd:"Move"
"Перемістити"

DeleteFileDelete
"&Удалить"
"&Delete"
"S&mazat"
"&Löschen"
"&Töröl"
"&Usuń"
"&Borrar"
"&Вилучити"

DeleteFileWipe
"&Уничтожить"
"&Wipe"
"V&ymazat"
"&Sicher löschen"
"Kisö&pör"
"&Wymaż"
"&Limpiar"
"&Знищити"

DeleteFileAll
"&Все"
"&All"
"&Vše"
"&Alle"
"Min&det"
"&wszystkie"
"&Todos"
"&Усе"

DeleteFileSkip
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

DeleteFileSkipAll
"П&ропустить все"
"S&kip all"
"Př&eskočit vše"
"A&lle überspr."
"Mind&et"
"O&miń wszystkie"
"O&mitir todos"
"П&ропустити все"

DeleteFileCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

DeletingTitle
l:
"Удаление"
"Deleting"
"Mazání"
"Lösche"
"Törlés"
"Usuwam"
"Borrando"
"Видалення"

Deleting
l:
"Удаление файла или папки"
"Deleting the file or folder"
"Mazání souboru nebo adresáře"
"Löschen von Datei oder Ordner"
"Fájl vagy mappa törlése"
"Usuwam plik/katalog"
"Borrando el archivo o directorio"
"Видалення файлу або теки"

DeletingWiping
"Уничтожение файла или папки"
"Wiping the file or folder"
"Vymazávání souboru nebo adresáře"
"Sicheres löschen von Datei oder Ordner"
"Fájl vagy mappa kisöprése"
"Wymazuję plik/katalog"
"Limpiando el archivo o directorio"
"Знищення файлу або теки"

DeleteRO
l:
"Файл имеет атрибут \"Только для чтения\""
"The file is read only"
"Soubor je určen pouze pro čtení"
"Folgende Datei ist schreibgeschützt:"
"A fájl csak olvasható:"
"Ten plik jest tylko do odczytu"
"El archivo es de sólo lectura"
"Файл має атрибут \"Тільки для читання\""

AskDeleteRO
"Вы хотите удалить его?"
"Do you wish to delete it?"
"Opravdu si ho přejete smazat?"
"Wollen Sie sie dennoch löschen?"
"Mégis törölni akarja?"
"Czy chcesz go usunąć?"
"Desea borrarlo?"
"Ви хочете видалити його?"

AskWipeRO
"Вы хотите уничтожить его?"
"Do you wish to wipe it?"
"Opravdu si ho přejete vymazat?"
"Wollen Sie sie dennoch sicher löschen?"
"Mégis ki akarja söpörni?"
"Czy chcesz go wymazać?"
"Desea limpiarlo?"
"Ви хочете знищити його?"

DeleteHardLink1
l:
"Файл имеет несколько жёстких ссылок"
"Several hard links link to this file."
"Více pevných linků ukazuje na tento soubor."
"Mehrere Hardlinks zeigen auf diese Datei."
"Több hardlink kapcsolódik a fájlhoz, a fájl"
"Do tego pliku prowadzi wiele linków trwałych."
"Demasiados enlaces rígidos a este archivo."
"Файл має кілька жорстких посилань"

DeleteHardLink2
"Уничтожение файла приведёт к обнулению всех ссылающихся на него файлов."
"Wiping this file will void all files linking to it."
"Vymazání tohoto souboru zneplatní všechny soubory, které na něj linkují."
"Sicheres Löschen dieser Datei entfernt ebenfalls alle Links."
"kisöprése a linkelt fájlokat is megsemmisíti."
"Wymazanie tego pliku wymaże wszystkie pliki dolinkowane."
"Limpiando este archivo invalidará todos los archivos enlazados."
"Знищення файлу призведе до обнулення всіх файлів, що посилаються."

DeleteHardLink3
"Уничтожать файл?"
"Do you wish to wipe this file?"
"Opravdu chcete vymazat tento soubor?"
"Wollen Sie diese Datei sicher löschen?"
"Biztosan kisöpri a fájlt?"
"Czy wymazać plik?"
"Desea limpiar este archivo"
"Знищувати файл?"

CannotDeleteFile
l:
"Ошибка удаления файла"
"Cannot delete the file"
"Nelze smazat soubor"
"Datei konnte nicht gelöscht werden"
"A fájl nem törölhető"
"Nie mogę usunąć pliku"
"No se puede borrar el archivo"
"Помилка видалення файлу"

CannotDeleteFolder
"Ошибка удаления папки"
"Cannot delete the folder"
"Nelze smazat adresář"
"Ordner konnte nicht gelöscht werden"
"A mappa nem törölhető"
"Nie mogę usunąć katalogu"
"No se puede borrar el directorio"
"Помилка видалення теки"

DeleteRetry
"&Повторить"
"&Retry"
"&Znovu"
"Wiede&rholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

DeleteSkip
"П&ропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"Po&miń"
"&Omitir"
"П&ропустити"

DeleteSkipAll
"Пропустить &все"
"S&kip all"
"Přeskočit &vše"
"A&lle überspr."
"Min&d"
"Pomiń &wszystkie"
"Omitir &Todo"
"Пропустити &все"

DeleteCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

EditTitle
l:
"Редактор"
"Editor"
"Editor"
"Editor"
"Szerkesztő"
"Edytor"
"Editor"
"Редактор"

AskReload
"уже загружен. Как открыть этот файл?"
"already loaded. How to open this file?"
"již otevřen. Jak otevřít tento soubor?"
"bereits geladen. Wie wollen Sie die Datei öffnen?"
"fájl már be van töltve. Hogyan szerkeszti?"
"został już załadowany. Załadować ponownie?"
"ya está cargado. Como abrir este archivo?"
"вже завантажено. Як відкрити цей файл?"

Current
"&Текущий"
"&Current"
"&Stávající"
"A&ktuell"
"A mostanit &folytatja"
"&Bieżący"
"A&ctual"
"&Поточний"

Reload
"Пере&грузить"
"R&eload"
"&Znovu načíst"
"Aktualisie&ren"
"Újra&tölti"
"&Załaduj"
"R&ecargar"
"Пере&вантажити"

NewOpen
"&Новая копия"
"&New instance"
"&Nová instance"
"&Neue Instanz"
"Ú&j példányban"
"&Nowa instancja"
"&Nueva instancia"
"&Нова копія"

EditCannotOpen
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie mogę otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

EditReading
"Чтение файла"
"Reading the file"
"Načítám soubor"
"Lesen der Datei"
"Fájl olvasása"
"Czytam plik"
"Leyendo el archivo"
"Читання файлу"

EditAskSave
"Файл был изменён. Сохранить?"
"File has been modified. Save?"
upd:"Soubor byl modifikován. Save?"
upd:"Datei wurde verändert. Save?"
upd:"A fájl megváltozott. Save?"
upd:"Plik został zmodyfikowany. Save?"
"El archivo ha sido modificado. Desea guardarlo?"
"Файл було змінено. Зберегти?"

EditAskSaveExt
"Файл был изменён внешней программой. Сохранить?"
"The file was changed by an external program. Save?"
upd:"Soubor byl změněný externím programem. Save?"
upd:"Die Datei wurde durch ein externes Programm verändert. Save?"
upd:"A fájlt egy külső program megváltoztatta. Save?"
upd:"Plik został zmieniony przez inny program. Save?"
"El archivo ha sido cambiado por un programa externo. Desea guardarlo?"
"Файл було змінено зовнішньою програмою. Зберегти?"

EditBtnSaveAs
"Сохр&анить как..."
"Save &as..."
"Ulož&it jako..."
"Speichern &als..."
"Mentés más&ként..."
"Zapisz &jako..."
"Guardar como..."
"Збе&регти як..."

EditRO
l:
"имеет атрибут \"Только для чтения\""
"is a read-only file"
"je určen pouze pro čtení"
"ist eine schreibgeschützte Datei"
"csak olvasható fájl"
"jest plikiem tylko do odczytu"
"es un archivo de sólo lectura"
"має атрибут \"Тільки для читання\""

EditExists
"уже существует"
"already exists"
"již existuje"
"ist bereits vorhanden"
"már létezik"
"już istnieje"
"ya existe"
"вже існує"

EditOvr
"Вы хотите перезаписать его?"
"Do you wish to overwrite it?"
"Přejete si ho přepsat?"
"Wollen Sie die Datei überschreiben?"
"Felül akarja írni?"
"Czy chcesz go nadpisać?"
"Desea sobrescribirlo?"
"Ви хочете перезаписати його?"

EditSaving
"Сохранение файла"
"Saving the file"
"Ukládám soubor"
"Speichere die Datei"
"Fájl mentése"
"Zapisuję plik"
"Guardando el archivo"
"Збереження файлу"

EditStatusLine
"Строка"
"Line"
"Řádek"
"Zeile"
"Sor"
"linia"
"Línea"
"Рядок"

EditStatusCol
"Кол"
"Col"
"Sloupec"
"Spal"
"Oszlop"
"kolumna"
"Col"
"Кол"

EditRSH
l:
"предназначен только для чтения"
"is a read-only file"
"je určen pouze pro čtení"
"ist eine schreibgeschützte Datei"
"csak olvasható fájl"
"jest plikiem tylko do odczytu"
"es un archivo de sólo lectura"
"призначений лише для читання"

EditFileGetSizeError
"Не удалось определить размер."
"File size could not be determined."
upd:"File size could not be determined."
upd:"File size could not be determined."
"A fájlméret megállapíthatatlan."
upd:"File size could not be determined."
"Tamaño de archivo no puede ser determinado"
"Не вдалося визначити розмір."

EditFileLong
"имеет размер %ls,"
"has the size of %ls,"
"má velikost %ls,"
"hat eine Größe von %ls,"
"mérete %ls,"
"ma wielkość %ls,"
"tiene el tamaño de %ls,"
"має розмір %ls,"

EditFileLong2
"что превышает заданное ограничение в %ls."
"which exceeds the configured maximum size of %ls."
"která překračuje nastavenou maximální velikost %ls."
"die die konfiguierte Maximalgröße von %ls überschreitet."
"meghaladja %ls beállított maximumát."
"przekraczającą ustalone maksimum %ls."
"cual excede el tamaño máximo configurado de %ls."
"що перевищує задане обмеження у %ls."

EditROOpen
"Вы хотите редактировать его?"
"Do you wish to edit it?"
"Opravdu si ho přejete upravit?"
"Wollen Sie sie dennoch bearbeiten?"
"Mégis szerkeszti?"
"Czy chcesz go edytować?"
"Desea editarlo?"
"Ви хочете редагувати його?"

EditCanNotEditDirectory
l:
"Невозможно редактировать папку"
"It is impossible to edit the folder"
"Nelze editovat adresář"
"Es ist nicht möglich den Ordner zu bearbeiten"
"A mappa nem szerkeszthető"
"Nie można edytować folderu"
"Es imposible editar el directorio"
"Неможливо редагувати теку"

EditSearchTitle
l:
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

EditSearchFor
"&Искать"
"&Search for"
"&Hledat"
"&Suchen nach"
"&Keresés:"
"&Znajdź"
"&Buscar por"
"&Шукати"

EditSearchCase
"&Учитывать регистр"
"&Case sensitive"
"&Rozlišovat velikost písmen"
"G&roß-/Kleinschrb."
"&Nagy/kisbetű érz."
"&Uwzględnij wielkość liter"
"Sensible min/ma&y"
"&Враховувати регістр"

EditSearchWholeWords
"Только &целые слова"
"&Whole words"
"&Celá slova"
"&Ganze Wörter"
"Csak e&gész szavak"
"Tylko całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

EditSearchReverse
"Обратн&ый поиск"
"Re&verse search"
"&Zpětné hledání"
"Richtung um&kehren"
"&Visszafelé keres"
"Szukaj w &odwrotnym kierunku"
"Búsqueda in&versa"
"Зворотн&ий пошук"

EditSearchSelFound
"&Выделять найденное"
"Se&lect found"
"Vy&ber nalezené"
"Treffer &markieren"
"&Találat kijelölése"
"W&ybierz znalezione"
"Se&leccionado encontrado"
"&Виділяти знайдене"

EditSearchRegexp
"&Регулярные выражения"
"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
"Expresiones re&gulares"
"&Регулярні вирази"

EditSearchSearch
"Искать"
"Search"
"Hledat"
"Suchen"
"Kere&sés"
"&Szukaj"
"Buscar"
"Шукати"

EditSearchCancel
"Отменить"
"Cancel"
"Storno"
"Abbruch"
"&Mégsem"
"&Anuluj"
"Cancelar"
"Скасувати"

EditReplaceTitle
l:
"Замена"
"Replace"
"Nahradit"
"Ersetzen"
"Keresés és csere"
"Zamień"
"Reemplazar"
"Заміна"

EditReplaceWith
"Заменить &на"
"R&eplace with"
"Nahradit &s"
"&Ersetzen mit"
"&Erre cseréli:"
"Zamień &na"
"R&eemplazar con"
"Замінити &на"

EditReplaceReplace
"&Замена"
"&Replace"
"&Nahradit"
"E&rsetzen"
"&Csere"
"Za&mień"
"&Reemplazar"
"&Заміна"

EditSearchingFor
l:
"Искать"
"Searching for"
"Vyhledávám"
"Suche nach"
"Keresett szöveg:"
"Szukam"
"Buscando por"
"Шукати"

EditNotFound
"Строка не найдена"
"Could not find the string"
"Nemůžu najít řetězec"
"Konnte Zeichenkette nicht finden"
"A szöveg nem található:"
"Nie mogę odnaleźć ciągu"
"No se puede encontrar la cadena"
"Рядок не знайдено"

EditAskReplace
l:
"Заменить"
"Replace"
"Nahradit"
"Ersetze"
"Ezt cseréli:"
"Zamienić"
"Reemplazar"
"Замінити"

EditAskReplaceWith
"на"
"with"
"s"
"mit"
"erre a szövegre:"
"na"
"con"
"на"

EditReplace
"&Заменить"
"&Replace"
"&Nahradit"
"E&rsetzen"
"&Csere"
"&Zamień"
"&Reemplazar"
"&Замінити"

EditReplaceAll
"&Все"
"&All"
"&Vše"
"&Alle"
"&Mindet"
"&Wszystkie"
"&Todos"
"&Усе"

EditSkip
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

EditCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"Mé&gsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

EditOpenCreateLabel
"&Открыть/создать файл:"
"&Open/create file:"
"Otevřít/vytvořit soubor:"
"Öffnen/datei erstellen:"
"Fájl megnyitása/&létrehozása:"
"&Otwórz/utwórz plik:"
"&Abrir/crear archivo:"
"&Відкрити/створити файл:"

EditOpenAutoDetect
"&Автоматическое определение"
"&Automatic detection"
upd:"Automatic detection"
upd:"Automatic detection"
"&Automatikus felismerés"
"&Wykryj automatycznie"
"Deteccion &automática"
"&Автоматичне визначення"

EditGoToLine
l:
"Перейти"
"Go to position"
"Jít na pozici"
"Gehe zu Zeile"
"Sorra ugrás"
"Idź do linii"
"Ir a posición"
"Перейти"

BookmarksTitle
l:
"Закладки"
"Bookmarks"
"Adresářové zkratky"
"Ordnerschnellzugriff"
"Mappa gyorsbillentyűk"
"Skróty katalogów"
"Accesos a directorio"
"Закладки"

PluginsTitle
l:
"Плагины"
"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
"Плагіни"

VTStop
l:
"Завершение фоновой оболочки."
"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
"Завершення фонової оболонки."

VTStopTip
l:
"Подсказка: чтобы закрыть far2m - введите 'exit far'."
"TIP: To close far2m - type 'exit far'."
upd:"TIP: To close far2m - type 'exit far'."
upd:"TIP: To close far2m - type 'exit far'."
upd:"TIP: To close far2m - type 'exit far'."
upd:"TIP: To close far2m - type 'exit far'."
upd:"TIP: To close far2m - type 'exit far'."
"Підказка: щоб закрити far2m - введіть 'exit far'."

VTStartTipNoCmdTitle
l:
"При наборе команды:                                                       "
"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
"При наборі команди:                                                       "

VTStartTipNoCmdCtrlO
l:
" Ctrl+O - переключения панель/терминал.                                   "
" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
" Ctrl+O - перемикання панель/термінал.                                    "

VTStartTipNoCmdCtrlArrow
l:
" Ctrl+Вверх/+Вниз/+Влево/+Вправо - изменение размера панелей.             "
" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
" Ctrl+Вгору/+Вниз/+Вліво/+Вправо - зміна розміру панелей.                 "

VTStartTipNoCmdShiftTAB
l:
" Двойной Shift+TAB - автодополнение от bash.                              "
" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
"Подвійний Shift+TAB - автодоповнення від bash.                            "

VTStartTipNoCmdFn
l:
" F3, F4, F8 - просмотр/редактор/очистка лога терминала при выкл. панелях. "
" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
" F3, F4, F8 - перегляд/редактор/очищення лога терміналу при вимкн. панелях."

VTStartTipNoCmdMouse
l:
" Ctrl+Shift+MouseScrollUp - автозавершающийся просмотр лога терминала.    "
" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
" Ctrl+Shift+MouseScrollUp - автозавершення перегляду лога терміналу.      "

VTStartTipPendCmdTitle
l:
"В процессе исполнения команды:                                            "
"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
"У процесі виконання команди:                                              "

VTStartTipPendCmdFn
l:
" Ctrl+Shift+F3/+F4 - пауза и открытие просмотра/редактора лога терминала. "
" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
" Ctrl+Shift+F3/+F4 - пауза та відкриття перегляду/редактора лога терміналу."

VTStartTipPendCmdCtrlAltC
l:
" Ctrl+Alt+C - завершить все процессы в этой оболочке.                     "
" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
" Ctrl+Alt+C - завершити всі процеси в цій оболонці.                       "

VTStartTipPendCmdCtrlAltZ
l:
" Ctrl+Alt+Z - отправить процесс far2m в фон, освободив терминал.          "
" Ctrl+Alt+Z - detach far2m application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2m application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2m application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2m application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2m application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2m application to background releasing terminal.  "
" Ctrl+Alt+Z - надіслати процес far2m у фон, звільнивши термінал.          "

VTStartTipPendCmdMouse
l:
" MouseScrollUp - автозавершающийся просмотр лога терминала.               "
" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
" MouseScrollUp - перегляд лога терміналу, що завершується автоматично.    "

BookmarkBottom
"Редактирование: Del,Ins,F4,Shift+Вверх,Shift+Вниз"
"Edit: Del,Ins,F4,Shift+Up,Shift+Down"
"Edit: Del,Ins,F4,Shift+Up,Shift+Down"
"Bearb.: Entf,Einf,F4,Shift+Up,Shift+Down"
"Szerk.: Del,Ins,F4,Shift+Up,Shift+Down"
"Edycja: Del,Ins,F4,Shift+Up,Shift+Down"
"Editar: Del,Ins,F4,Shift+Up,Shift+Down"
"Редагування: Del,Ins,F4,Shift+Вгору,Shift+Вниз"

ShortcutNone
"<отсутствует>"
"<none>"
"<není>"
"<keiner>"
"<nincs>"
"<brak>"
"<nada>"
"<відсутня>"

ShortcutPlugin
"<плагин>"
"<plugin>"
"<plugin>"
"<Plugin>"
"<plugin>"
"<plugin>"
"<plugin>"
"<плагін>"

FSShortcut
"Введите новую закладку:"
"Enter bookmark path:"
"Zadejte novou zkratku:"
"Neue Verknüpfung:"
"A gyorsbillentyűhöz rendelt mappa:"
"Wprowadź nowy skrót:"
"Ingrése nuevo acceso:"
"Введіть нову закладку:"

NeedNearPath
"Перейти в ближайшую доступную папку?"
"Jump to the nearest existing folder?"
"Skočit na nejbližší existující adresář?"
"Zum nahesten existierenden Ordner springen?"
"Ugrás a legközelebbi létező mappára?"
"Przejść do najbliższego istniejącego folderu?"
"Saltar al próximo directorio existente"
"Перейти до доступної теки?"

SaveThisShortcut
"Запомнить эту закладку?"
"Save this bookmark?"
"Uložit tyto zkratky?"
"Verknüpfung speichern?"
"Mentsem a gyorsbillentyűket?"
"Zapisać skróty?"
"Guardar estos accesos"
"Запам'ятати цю закладку?"

EditF1
l:
l://functional keys - 6 characters max, 12 keys, "OEM" is F8 dupe!
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

EditF8
"->ANSI"
"->ANSI"
"->ANSI"
"->ANSI"
"->ANSI"
"Latin 2"
"->ANSI"
"->ANSI"

EditF8DOS
le:// don't count this - it's a F8 another text
"->OEM"
"->OEM"
"->OEM"
"->OEM"
"->OEM"
"CP-1250"
"->OEM"
"->OEM"

EditF8UTF8
le:// don't count this - it's a F8 another text
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"

ViewF5Processed
le:// don't count this - it's a F5 another text
"Обработ"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Обробно"

ViewF5Raw
le:// don't count this - it's a F5 another text
"Сырой"
"Raw"
"Raw"
"Raw"
"Raw"
"Raw"
"Raw"
"Сирий"

EditShiftF1
l:
l://Editor: Shift
""
""
""
""
""
""
""
""

EditShiftF2
"Сохр.в"
"SaveAs"
"UlJako"
"SpeiUn"
"Ment.."
"Zapisz"
"Grbcom"
"Збер.в"

EditAltF1
l:
l://Editor: Alt
""
""
""
""
""
""
""
""

EditCtrlF1
l:
l://Editor: Ctrl
""
""
""
""
""
""
""
""

EditAltShiftF1
l:
l://Editor: AltShift
""
""
""
""
""
""
""
""

EditCtrlShiftF1
l:
l://Editor: CtrlShift
""
""
""
""
""
""
""
""

EditCtrlAltF1
l:
l:// Editor: CtrlAlt
""
""
""
""
""
""
""
""

EditCtrlAltShiftF1
l:
l:// Editor: CtrlAltShift
""
""
""
""
""
""
""
""

SingleEditF1
l:
l://Single Editor: functional keys - 6 characters max, 12 keys, "OEM" is F8 dupe!
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допмга"

SingleEditF8
"->ANSI"
"->ANSI"
"->ANSI"
"->ANSI"
"->ANSI"
"Latin 2"
"->ANSI"
"->ANSI"

SingleEditF8DOS
le:// don't count this - it's a F8 another text
"->OEM"
"->OEM"
"->OEM"
"->OEM"
"->OEM"
"CP 1250"
"->OEM"
"->OEM"

SingleEditF8UTF8
le:// don't count this - it's a F8 another text
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"

SingleEditShiftF1
l:
l://Single Editor: Shift
""
""
""
""
""
""
""
""

SingleEditAltF1
l:
l://Single Editor: Alt
""
""
""
""
""
""
""
""

SingleEditCtrlF1
l:
l://Single Editor: Ctrl
""
""
""
""
""
""
""
""

SingleEditAltShiftF1
l:
l://Single Editor: AltShift
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF1
l:
l://Single Editor: CtrlShift
""
""
""
""
""
""
""
""

SingleEditCtrlAltF1
l:
l://Single Editor: CtrlAlt
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF1
l:
l://Single Editor: CtrlAltShift
""
""
""
""
""
""
""
""

EditSaveAs
l:
"Сохранить &файл как"
"Save file &as"
"Uložit soubor jako"
"Speichern &als"
"Fá&jl mentése, mint:"
"Zapisz plik &jako"
"Guardar archivo &como"
"Зберегти &файл як"

EditCodePage
"&Кодовая страница:"
"&Code page:"
"Kódová stránka:"
"Codepage:"
"Kódlap:"
"&Strona kodowa:"
"&Código caracteres:"
"&Кодова сторінка:"

EditAddSignature
"Добавить &сигнатуру (BOM)"
"Add &signature (BOM)"
"Přidat signaturu (BOM)"
"Sinatur hinzu (BOM)"
"Uni&code bájtsorrend jelzővel (BOM)"
"Dodaj &znacznik BOM"
"Añadir &signatura (BOM)"
"Додати &сигнатуру (BOM)"

EditSaveAsFormatTitle
"Изменить перевод строки:"
"Change line breaks to:"
"Změnit zakončení řádků na:"
"Zeilenumbrüche setzen:"
"Sortörés konverzió:"
"Zamień znaki końca linii na:"
"Cambiar fin de líneas a:"
"Змінити розриви рядків на:"

EditSaveOriginal
"&исходный формат"
"Do n&ot change"
"&Beze změny"
"Nicht verä&ndern"
"Nincs &konverzió"
"&Nie zmieniaj"
"N&o cambiar"
"&Вихідний формат"

EditSaveDOS
"в форма&те DOS/Windows (CR LF)"
"&Dos/Windows format (CR LF)"
"&Dos/Windows formát (CR LF)"
"&Dos/Windows Format (CR LF)"
"&DOS/Windows formátum (CR LF)"
"Format &Dos/Windows (CR LF)"
"Formato &DOS/Windows (CR LF)"
"у форма&ті DOS/Windows (CR LF)"

EditSaveUnix
"в формат&е UNIX (LF)"
"&Unix format (LF)"
"&Unix formát (LF)"
"&Unix Format (LF)"
"&UNIX formátum (LF)"
"Format &Unix (LF)"
"Formato &Unix (LF)"
"у формат&і UNIX (LF)"

EditSaveMac
"в фор&мате MAC (CR)"
"&Mac format (CR)"
"&Mac formát (CR)"
"&Mac Format (CR)"
"&Mac formátum (CR)"
"Format &Mac (CR)"
"Formato &Mac (CR)"
"у фор&маті MAC (CR)"

EditCannotSave
"Ошибка сохранения файла"
"Cannot save the file"
"Nelze uložit soubor"
"Kann die Datei nicht speichern"
"A fájl nem menthető"
"Nie mogę zapisać pliku"
"No se puede guardar archivo"
"Помилка збереження файлу"

EditSavedChangedNonFile
"Файл изменён, но файл или папка, в которой он находился,"
"The file is changed but the file or the folder containing"
"Soubor je změněn, ale soubor, nebo adresář obsahující"
"Inhalt dieser Datei wurde verändert aber die Datei oder der Ordner, welche"
"A fájl megváltozott, de a fájlt vagy a mappáját"
"Plik został zmieniony, ale plik lub folder zawierający"
"El archivo es cambiado pero el archivo o el directorio que contiene"
"Файл змінено, але файл або тека, в якій він знаходився,"

EditSavedChangedNonFile1
"Файл или папка, в которой он находился,"
"The file or the folder containing"
"Soubor nebo adresář obsahující"
"Die Datei oder der Ordner, welche"
"A fájlt vagy a mappáját"
"Plik lub folder zawierający"
"El archivo o el directorio conteniendo"
"Файл або тека, де він знаходився,"

EditSavedChangedNonFile2
"был перемещён или удалён. Сохранить?"
"this file was moved or deleted. Save?"
upd:"tento soubor byl přesunut, nebo smazán. Save?"
upd:"diesen Inhalt enthält wurde verschoben oder gelöscht. Save?"
upd:"időközben áthelyezte/átnevezte vagy törölte. Save?"
upd:"ten plik został przeniesiony lub usunięty. Save?"
"este archivo ha sido movido o borrado. Desea guardarlo?"
"Було переміщено або видалено. Зберегти?"

EditNewPath1
"Путь к редактируемому файлу не существует,"
"The path to the edited file does not exist,"
"Cesta k editovanému souboru neexistuje,"
"Der Pfad zur bearbeiteten Datei existiert nicht,"
"A szerkesztendő fájl célmappája még"
"Ścieżka do edytowanego pliku nie istnieje,"
"La ruta del archivo editado no existe,"
"Шлях до редагованого файлу не існує,"

EditNewPath2
"но будет создан при сохранении файла."
"but will be created when the file is saved."
"ale bude vytvořena při uložení souboru."
"aber wird erstellt sobald die Datei gespeichert wird."
"nem létezik, de mentéskor létrejön."
"ale zostanie utworzona po zapisaniu pliku."
"pero será creada cuando el archivo sea guardado."
"але буде створено при збереженні файлу."

EditNewPath3
"Продолжать?"
"Continue?"
"Pokračovat?"
"Fortsetzen?"
"Folytatja?"
"Kontynuować?"
"Continuar?"
"Продовжувати?"

EditNewPlugin1
"Имя редактируемого файла не может быть пустым"
"The name of the file to edit cannot be empty"
"Název souboru k editaci nesmí být prázdné"
"Der Name der zu editierenden Datei kann nicht leer sein"
"A szerkesztendő fájlnak nevet kell adni"
"Nazwa pliku do edycji nie może być pusta"
"El nombre del archivo a editar no puede estar vacío"
"Ім'я файлу, що редагується, не може бути порожнім"

EditorLoadCPWarn1
"Файл содержит символы, которые невозможно"
"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
"El archivo contiene caracteres que no pueden ser"
"Файл містить символи, які неможливо"

EditorLoadCPWarn2
"корректно прочитать, используя выбранную кодовую страницу."
"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
"correctamente leídos con la tabla (codepage) seleccionada."
"коректно прочитати, використовуючи вибрану кодову сторінку."

EditorSaveCPWarn1
"Редактор содержит символы, которые невозможно"
"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
"El editor contiene caracteres que no pueden ser"
"Редактор містить символи, які неможливо"

EditorSaveCPWarn2
"корректно сохранить, используя выбранную кодовую страницу."
"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
"correctamente guardados con la tabla (codepage) seleccionada."
"коректно зберегти, використовуючи вибрану кодову сторінку."

EditorSwitchCPWarn1
"Редактор содержит символы, которые невозможно"
"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
"El editor contiene caracteres que no pueden ser"
"Редактор містить символи, які неможливо"

EditorSwitchCPWarn2
"корректно преобразовать, используя выбранную кодовую страницу."
"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
"correctamente traducidos con la tabla (codepage) seleccionada."
"коректно перетворити, використовуючи вибрану кодову сторінку."

EditDataLostWarn
"Во время редактирования файла некоторые данные были утеряны."
"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
"Durante la edición del archivo algunos datos se perdieron."
"Під час редагування файлу деякі дані були втрачені."

EditorSaveNotRecommended
"Сохранять файл не рекомендуется."
"It is not recommended to save this file."
"Není doporučeno uložit tento soubor."
"Es wird empfohlen, die Datei nicht zu speichern."
"A fájl mentése nem ajánlott."
"Odradzamy zapis pliku."
"No se recomienda guardar este archivo."
"Не рекомендується зберігати файл."

EditorSaveCPWarnShow
"Показать"
"Show"
upd:"Show"
upd:"Show"
upd:"Show"
upd:"Show"
"Mostrar"
"Показати"

ColumnName
l:
"Имя"
"Name"
"Název"
"Name"
"Név"
"Nazwa"
"Nombre"
"Ім'я"

ColumnSize
"Размер"
"Size"
"Velikost"
"Größe"
"Méret"
"Rozmiar"
"Tamaño"
"Розмір"

ColumnPhysical
"ФизРзм"
"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
"ФізРзм"

ColumnDate
"Дата"
"Date"
"Datum"
"Datum"
"Dátum"
"Data"
"Fecha"
"Дата"

ColumnTime
"Время"
"Time"
"Čas"
"Zeit"
"Idő"
"Czas"
"Hora"
"Час"

ColumnWrited
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

ColumnCreated
"Создание"
"Created"
"Vytvořen"
"Erstellt"
"Létrejött"
"Utworzenie"
"Creado "
"Створення"

ColumnAccessed
"Доступ"
"Accessed"
"Přístup"
"Zugriff"
"Hozzáférés"
"Użycie"
"Acceso  "
"Доступ"

ColumnChanged
"Изменение"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
"Змінення"

ColumnAttr
"Атриб"
"Attr"
"Attr"
"Attr"
"Attrib"
"Atrybuty"
"Atrib"
"Атриб"

ColumnDescription
"Описание"
"Description"
"Popis"
"Beschreibung"
"Megjegyzés"
"Opis"
"Descripción"
"Опис"

ColumnOwner
"Владелец"
"Owner"
"Vlastník"
"Besitzer"
"Tulajdonos"
"Właściciel"
"Dueño"
"Власник"

ColumnGroup
"Группа"
"Group"
upd:"Group"
upd:"Group"
upd:"Group"
upd:"Group"
upd:"Group"
"Група"

ColumnMumLinks
"КлС"
"NmL"
"PočLn"
"AnL"
"Lnk"
"NmL"
"NmL"
"КлС"

ListUp
l:
"Вверх"
"  Up  "
"Nahoru"
" Hoch "
"  Fel  "
"W górę"
"UP-DIR"
"Вгору"

ListFolder
"Папка"
"Folder"
"Adresář"
"Ordner"
" Mappa "
"Folder"
" DIR  "
"Тека"

ListSymLink
"Ссылка"
"Symlink"
"Link"
"Symlink"
"SzimLnk"
"LinkSym"
" Enlac"
"Посилання"

ListBytes
"Б"
"B"
"B"
"B"
"B"
"B"
"B"
"Б"

ListMb
"М"
"M"
"M"
"M"
"M"
"M"
"M"
"М"

ListFileSize
" %ls байт в 1 файле "
" %ls bytes in 1 file "
" %ls bytů v 1 souboru "
" %ls Bytes in 1 Datei "
" %ls bájt 1 fájlban "
" %ls bajtów w 1 pliku "
" %ls bytes en 1 archivo "
" %ls байт у 1 файлі "

ListFilesSize1
" %ls байт в %d файле "
" %ls bytes in %d files "
" %ls bytů v %d souborech "
" %ls Bytes in %d Dateien "
" %ls bájt %d fájlban "
" %ls bajtów w %d plikach "
" %ls bytes en %d archivos "
" %ls байт у %d файлі "

ListFilesSize2
" %ls байт в %d файлах "
" %ls bytes in %d files "
" %ls bytů v %d souborech "
" %ls Bytes in %d Dateien "
" %ls bájt %d fájlban "
" %ls bajtów w %d plikach "
" %ls bytes en %d archivos "
" %ls байт у %d файлах "

ListFreeSize
" %ls байт свободно "
" %ls free bytes "
" %ls volných bytů "
" %ls freie Bytes "
" %ls szabad bájt "
" %ls wolnych bajtów "
" %ls bytes libres "
" %ls байт вільно "

DirInfoViewTitle
l:
"Просмотр"
"View"
"Zobraz"
"Betrachten"
"Vizsgálat"
"Podgląd"
"Ver "
"Перегляд"

FileToEdit
"Редактировать файл:"
"File to edit:"
"Soubor k editaci:"
"Datei bearbeiten:"
"Szerkesztendő fájl:"
"Plik do edycji:"
"archivo a editar:"
"Редагувати файл:"

UnselectTitle
l:
"Снять"
"Deselect"
"Odznačit"
"Abwählen"
"Kijelölést levesz"
"Odznacz"
"Deseleccionar"
"Зняти"

SelectTitle
"Пометить"
"Select"
"Označit"
"Auswählen"
"Kijelölés"
"Zaznacz"
"Seleccionar"
"Позначити"

SelectFilter
"&Фильтр"
"&Filter"
"&Filtr"
"&Filter"
"&Szűrő"
"&Filtruj"
"&Filtro"
"&Фільтр"

CompareTitle
l:
"Сравнение"
"Compare"
"Porovnat"
"Vergleichen"
"Összehasonlítás"
"Porównaj"
"Comparar"
"Порівняння"

CompareFilePanelsRequired1
"Для сравнения папок требуются"
"Two file panels are required to perform"
"Pro provedení příkazu Porovnat adresáře"
"Zwei Dateipanels werden benötigt um"
"Mappák összehasonlításához"
"Aby porównać katalogi konieczne są"
"Dos paneles de archivos son necesarios para poder"
"Для порівняння тек потрібні"

CompareFilePanelsRequired2
"две файловые панели"
"the Compare folders command"
"jsou nutné dva souborové panely"
"den Vergleich auszuführen."
"két fájlpanel szükséges"
"dwa zwykłe panele plików"
"utilizar el comando comparar directorios"
"дві файлові панелі"

CompareSameFolders1
"Содержимое папок,"
"The folders contents seems"
"Obsahy adresářů jsou"
"Der Inhalt der beiden Ordner scheint"
"A mappák tartalma"
"Zawartość katalogów"
"El contenido de los directorios parecen"
"Вміст тек,"

CompareSameFolders2
"скорее всего, одинаково"
"to be identical"
"identické"
"identisch zu sein."
"azonosnak tűnik"
"wydaje się być identyczna"
"ser idénticos"
"швидше за все, однаково"

SelectAssocTitle
l:
"Выберите ассоциацию"
"Select association"
"Vyber závislosti"
"Dateiverknüpfung auswählen"
"Válasszon társítást"
"Wybierz przypisanie"
"Seleccionar asociaciones"
"Виберіть асоціацію"

AssocTitle
l:
"Ассоциации для файлов"
"File associations"
"Závislosti souborů"
"Dateiverknüpfungen"
"Fájltársítások"
"Przypisania plików"
"Asociación de archivos"
"Асоціації для файлів"

AssocBottom
"Редактирование: Del,Ins,F4"
"Edit: Del,Ins,F4"
"Edit: Del,Ins,F4"
"Bearb.: Entf,Einf,F4"
"Szerk.: Del,Ins,F4"
"Edycja: Del,Ins,F4"
"Editar: Del,Ins,F4"
"Редагування: Del,Ins,F4"

AskDelAssoc
"Вы хотите удалить ассоциацию для"
"Do you wish to delete association for"
"Přejete si smazat závislost pro"
"Wollen Sie die Verknüpfung löschen für"
"Törölni szeretné a társítást?"
"Czy chcesz usunąć przypisanie dla"
"Desea borrar la asociación para"
"Ви хочете видалити асоціацію для"

FileAssocTitle
l:
"Редактирование ассоциаций файлов"
"Edit file associations"
"Upravit závislosti souborů"
"Dateiverknüpfungen bearbeiten"
"Fájltársítások szerkesztése"
"Edytuj przypisania pliku"
"Editar asociación de archivos"
"Редагування асоціацій файлів"

FileAssocMasks
"Одна или несколько &масок файлов:"
"A file &mask or several file masks:"
"&Maska nebo masky souborů:"
"Datei&maske (mehrere getrennt mit Komma):"
"F&ájlmaszk(ok, vesszővel elválasztva):"
"&Maska pliku lub kilka masek oddzielonych przecinkami:"
"&Máscara de archivo o múltiples máscaras de archivos:"
"Одна або кілька &масок файлів:"

FileAssocDescr
"&Описание ассоциации:"
"&Description of the association:"
"&Popis asociací:"
"&Beschreibung der Verknüpfung:"
"A &társítás leírása:"
"&Opis przypisania:"
"&Descripción de la asociación:"
"&Опис асоціації:"

FileAssocExec
"Команда, &выполняемая по Enter:"
"E&xecute command (used for Enter):"
"&Vykonat příkaz (použito pro Enter):"
"Befehl &ausführen (mit Enter):"
"&Végrehajtandó parancs (Enterre):"
"Polecenie (po naciśnięciu &Enter):"
"E&jecutar comando (usado por Enter):"
"Команда, &яка виконується за Enter:"

FileAssocAltExec
"Коман&да, выполняемая по Ctrl-PgDn:"
"Exec&ute command (used for Ctrl-PgDn):"
"V&ykonat příkaz (použito pro Ctrl-PgDn):"
"Befehl a&usführen (mit Strg-PgDn):"
"Vé&grehajtandó parancs (Ctrl-PgDown-ra):"
"Polecenie (po naciśnięciu &Ctrl-PgDn):"
"Ejecutar comando (usado por Ctrl-PgDn):"
"Коман&да, що виконується за Ctrl-PgDn:"

FileAssocView
"Команда &просмотра, выполняемая по F3:"
"&View command (used for F3):"
"Příkaz &Zobraz (použito pro F3):"
"Be&trachten (mit F3):"
"&Nézőke parancs (F3-ra):"
"&Podgląd (po naciśnięciu F3):"
"Comando de &visor (usado por F3):"
"Команда &перегляду, що виконується за F3:"

FileAssocAltView
"Команда просмотра, в&ыполняемая по Alt-F3:"
"V&iew command (used for Alt-F3):"
"Příkaz Z&obraz (použito pro Alt-F3):"
"Bet&rachten (mit Alt-F3):"
"N&ézőke parancs (Alt-F3-ra):"
"Podg&ląd (po naciśnięciu Alt-F3):"
"Comando de visor (usado por Alt-F3):"
"Команда перегляду, що в&иконується за Alt-F3:"

FileAssocEdit
"Команда &редактирования, выполняемая по F4:"
"&Edit command (used for F4):"
"Příkaz &Edituj (použito pro F4):"
"Bearb&eiten (mit F4):"
"S&zerkesztés parancs (F4-re):"
"&Edycja  (po naciśnięciu F4):"
"Comando de &editor (usado por F4):"
"Команда &редагування, що виконується за F4:"

FileAssocAltEdit
"Команда редактировани&я, выполняемая по Alt-F4:"
"Edit comm&and (used for Alt-F4):"
"Příkaz Editu&j (použito pro Alt-F4):"
"Bearbe&iten (mit Alt-F4):"
"Sze&rkesztés parancs (Alt-F4-re):"
"E&dycja  (po naciśnięciu Alt-F4):"
"Comando de editor (usado por Alt-F4):"
"Команда редагуванн&я, що виконується за Alt-F4:"
ViewF1
l:
l://Viewer: functional keys, 12 keys, except F2 - 2 keys, and F8 - 2 keys
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допмга"

ViewF2
le:// this is another text for F2
"Сверн"
"Wrap"
"Zalom"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

ViewF4
"Код"
"Hex"
"Hex"
"Hex"
"Hexa"
"Hex"
"Hexa"
"Код"

ViewF8
"->ANSI"
"->ANSI"
"->ANSI"
"->ANSI"
"->ANSI"
"Latin 2"
"->ANSI"
"->ANSI"

ViewF2Unwrap
"Развер"
"Unwrap"
"Nezal"
"KeinUm"
"NemTör"
"Unwrap"
"Unwrap"
"Розгор"

ViewF4Text
l:// this is another text for F4
"Текст"
"Text"
"Text"
"Text"
"Szöveg"
"Tekst"
"Text"
"Текст"

ViewF8DOS
"->OEM"
"->OEM"
"->OEM"
"->OEM"
"->OEM"
"CP 1250"
"->OEM"
"->OEM"

ViewF8UTF8
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"
"->UTF8"

ViewShiftF1
l:
l://Viewer: Shift
""
""
""
""
""
""
""
""

ViewShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"ZawińS"
"ConDiv"
"Слова"

ViewAltF1
l:
l://Viewer: Alt
""
""
""
""
""
""
""
""

ViewCtrlF1
l:
l://Viewer: Ctrl
""
""
""
""
""
""
""
""

ViewAltShiftF1
l:
l://Viewer: AltShift
""
""
""
""
""
""
""
""

ViewCtrlShiftF1
l:
l://Viewer: CtrlShift
""
""
""
""
""
""
""
""

ViewCtrlAltF1
l:
l://Viewer: CtrlAlt
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF1
l:
l://Viewer: CtrlAltShift
""
""
""
""
""
""
""
""

SingleViewF1
l:
l://Single Viewer: functional keys, 12 keys, except F2 - 2 keys, and F8 - 2 keys
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допмга"

SingleViewShiftF1
l:
l://Single Viewer: Shift
""
""
""
""
""
""
""
""

SingleViewAltF1
l:
l://Single Viewer: Alt
""
""
""
""
""
""
""
""

SingleViewCtrlF1
l:
l://Single Viewer: Ctrl
""
""
""
""
""
""
""
""

SingleViewAltShiftF1
l:
l://Single Viewer: AltShift
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF1
l:
l://Single Viewer: CtrlShift
""
""
""
""
""
""
""
""

SingleViewCtrlAltF1
l:
l://Single Viewer: CtrlAlt
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF1
l:
l://Single Viewer: CtrlAltShift
""
""
""
""
""
""
""
""

InViewer
"просмотр %ls"
"view %ls"
"prohlížení %ls"
"Betrachte %ls"
"%ls megnézése"
"podgląd %ls"
"ver %ls"
"перегляд %ls"

InEditor
"редактирование %ls"
"edit %ls"
"editace %ls"
"Bearbeite %ls"
"%ls szerkesztése"
"edycja %ls"
"editar %ls"
"редагування %ls"

FilterTitle
l:
"Меню фильтров"
"Filters menu"
"Menu filtrů"
"Filtermenü"
"Szűrők menü"
"Filtry"
"Menú de Filtros"
"Меню фільтрів"

FilterBottom
"+,-,Пробел,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Space,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Mezera,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Leer,I,X,BS,UmschBS,Einf,Entf,F4,F5,StrgUp,StrgDn"
"+,-,Szóköz,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Fel,Ctrl-Le"
"+,-,Spacja,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"Seleccione: '+','-',Space. Editor: Ins,Del,F4"
"+,-,Пробіл,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"

PanelFileType
"Файлы панели"
"Panel file type"
"Typ panelu souborů"
"Dateityp in Panel"
"A panel fájltípusa"
"Typ plików w panelu"
"Tipo de panel de archivo"
"Файли панелі"

FolderFileType
"Папки"
"Folders"
"Adresáře"
"Ordner"
"Mappák"
"Foldery"
"Directorios"
"Теки"

CanEditCustomFilterOnly
"Только пользовательский фильтр можно редактировать"
"Only custom filter can be edited"
"Jedině vlastní filtr může být upraven"
"Nur eigene Filter können editiert werden."
"Csak saját szűrő szerkeszthető"
"Tylko filtr użytkownika może być edytowany"
"Sólo filtro personalizado puede ser editado"
"Тільки фільтр користувача можна редагувати"

AskDeleteFilter
"Вы хотите удалить фильтр"
"Do you wish to delete the filter"
"Přejete si smazat filtr"
"Wollen Sie den eigenen Filter löschen"
"Törölni szeretné a szűrőt?"
"Czy chcesz usunąć filtr"
"Desea borrar el filtro"
"Ви хочете видалити фільтр"

CanDeleteCustomFilterOnly
"Только пользовательский фильтр может быть удалён"
"Only custom filter can be deleted"
"Jedině vlastní filtr může být smazán"
"Nur eigene Filter können gelöscht werden."
"Csak saját szűrő törölhető"
"Tylko filtr użytkownika może być usunięty"
"Sólo filtro personalizado puede ser borrado"
"Тільки фільтр користувача може бути видалений"

FindFileTitle
l:
"Поиск файла"
"Find file"
"Hledat soubor"
"Nach Dateien suchen"
"Fájlkeresés"
"Znajdź plik"
"Encontrar archivo"
"Пошук файлу"

FindFileMasks
"Одна или несколько &масок файлов:"
"A file &mask or several file masks:"
"Maska nebo masky souborů:"
"Datei&maske (mehrere getrennt mit Komma):"
"Fájlm&aszk(ok, vesszővel elválasztva):"
"&Maska pliku lub kilka masek oddzielonych przecinkami:"
"&Máscara de archivo o múltiples máscaras de archivos:"
"Одна або кілька &масок файлів:"

FindFileText
"&Содержащих текст:"
"Con&taining text:"
"Obsahující te&xt:"
"Enthält &Text:"
"&Tartalmazza a szöveget:"
"Zawierający &tekst:"
"Conteniendo &texto:"
"&Той, що містить текст:"

FindFileHex
"&Содержащих 16-ричный код:"
"Con&taining hex:"
"Obsahující &hex:"
"En&thält Hex (xx xx ...):"
"Tartalmazza a he&xát:"
"Zawierający wartość &szesnastkową:"
"Conteniendo Hexa:"
"&Вміст, що містить 16-річний код:"

FindFileCodePage
"Используя кодо&вую страницу:"
"Using code pa&ge:"
upd:"Použít &znakovou sadu:"
upd:"Zeichenta&belle verwenden:"
"Kó&dlap:"
"Użyj tablicy znaków:"
"Usando tabla de caracteres:"
"Використовуючи кодо&ву сторінку:"

FindFileCodePageBottom
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Espacio, Ins"
"Space, Ins"

FindFileCase
"&Учитывать регистр"
"&Case sensitive"
"Roz&lišovat velikost písmen"
"Gr&oß-/Kleinschreibung"
"&Nagy/kisbetű érzékeny"
"&Uwzględnij wielkość liter"
"Sensible min/ma&yúsc."
"&Враховувати регістр"

FindFileWholeWords
"Только &целые слова"
"&Whole words"
"&Celá slova"
"Nur &ganze Wörter"
"Csak egés&z szavak"
"Tylko &całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

FindFileAllCodePages
"Все кодовые страницы"
"All code pages"
upd:"Všechny znakové sady"
upd:"Alle Zeichentabellen"
"Minden kódlappal"
"Wszystkie zainstalowane"
"Todas las tablas de caracteres"
"Всі кодові сторінки"

FindArchives
"Искать в а&рхивах"
"Search in arch&ives"
"Hledat v a&rchívech"
"In Arch&iven suchen"
"Keresés t&ömörítettekben"
"Szukaj w arc&hiwach"
"Buscar en archivos compr&imidos"
"Шукати в а&рхівах"

FindFolders
"Искать п&апки"
"Search for f&olders"
"Hledat a&dresáře"
"Nach &Ordnern suchen"
"Keresés mapp&ákra"
"Szukaj &folderów"
"Buscar por direct&orios"
"Шукати т&еки"

FindSymLinks
"Искать в символи&ческих ссылках"
"Search in symbolic lin&ks"
"Hledat v s&ymbolických lincích"
"In symbolischen Lin&ks suchen"
"Keresés sz&imbolikus linkekben"
"Szukaj w &linkach"
"Buscar en enlaces simbólicos"
"Шукати в символі&чних посиланнях"

SearchForHex
"Искать 16-ричн&ый код"
"Search for &hex"
"Hledat &hex"
"Nach &Hex suchen"
"Keresés &hexákra"
"Szukaj wartości &szesnastkowej"
"Buscar por &hexa"
"Шукати 16-річн&ий код"

SearchWhere
"Выберите &область поиска:"
"Select search &area:"
upd:"Zvolte oblast hledání:"
upd:"Suchbereich:"
"Keresés hatós&ugara:"
"Obszar wyszukiwania:"
"Seleccionar área de búsqueda:"
"Виберіть &область пошуку:"

SearchAllDisks
"На всех несъёмных &дисках"
"In &all non-removable drives"
"Ve všech p&evných discích"
"Auf &allen festen Datenträger"
"Minden &fix meghajtón"
"Na dyskach &stałych"
"Buscar en todas las unidades no-removibles"
"На всіх незнімних &дисках"

SearchAllButNetwork
"На всех &локальных дисках"
"In all &local drives"
"Ve všech &lokálních discích"
"Auf allen &lokalen Datenträgern"
"Minden hel&yi meghajtón"
"Na dyskach &lokalnych"
"Buscar en todas las unidades locales"
"На всіх &локальних дисках"

SearchInPATH
"В PATH-катало&гах"
"In &PATH folders"
"V adresářích z &PATH"
"In &PATH-Ordnern"
"A &PATH mappáiban"
"W folderach zmiennej &PATH"
"En directorios de variable &PATH"
"У PATH-катало&гах"

SearchFromRootFolder
"С кор&невой папки"
"From the &root folder"
"V kořeno&vém adresáři"
"Ab Wu&rzelverzeichnis"
"A &gyökérmappától"
"Od katalogu &głównego"
"Buscar desde la &raíz del directorio"
"З кор&еневої теки"

SearchFromCurrent
"С &текущей папки"
"From the curre&nt folder"
"V tomto adresář&i"
"Ab dem aktuelle&n Ordner"
"Az akt&uális mappától"
"Od &bieżącego katalogu"
"Buscar desde directorio actual"
"З &поточної теки"

SearchInCurrent
"Только в теку&щей папке"
"The current folder onl&y"
"P&ouze v tomto adresáři"
"Nur im aktue&llen Ordner"
"&Csak az aktuális mappában"
"&Tylko w bieżącym katalogu"
"Buscar en el directorio actua&l solamente"
"Тільки в пото&чній теці"

SearchInSelected
"В &отмеченных папках"
"&Selected folders"
"Ve vy&braných adresářích"
"In au&sgewählten Ordner"
"A ki&jelölt mappákban"
"W &zaznaczonych katalogach"
"Buscar en directorios &seleccionados"
"У &відмічених теках"

FindUseFilter
"Исполь&зовать фильтр"
"&Use filter"
"Použít f&iltr"
"Ben&utze Filter"
"Sz&űrővel"
"&Filtruj"
"&Usar filtro"
"Викорис&товувати фільтр"

FindUsingFilter
"используя фильтр"
"using filter"
"používám filtr"
"mit Filter"
"szűrővel"
"używając filtra"
"usando filtro"
"використовуючи фільтр"

FindFileFind
"&Искать"
"&Find"
"&Hledat"
"&Suchen"
"K&eres"
"Szuka&j"
"&Encontrar"
"&Шукати"

FindFileDrive
"Дис&к"
"Dri&ve"
"D&isk"
"Lauf&werk"
"Meghajt&ó"
"&Dysk"
"Uni&dad"
"Дис&к"

FindFileSetFilter
"&Фильтр"
"Filt&er"
"&Filtr"
"Filt&er"
"Szű&rő"
"&Filtr"
"Filtr&o"
"&Фільтр"

FindFileAdvanced
"До&полнительно"
"Advance&d"
"Pokr&očilé"
"Er&weitert"
"Ha&ladó"
"&Zaawansowane"
"Avanza&da"
"До&датково"

FindSearchingIn
"Поиск%ls в"
"Searching%ls in"
"Hledám%ls v"
"Suche%ls in"
"%ls keresése"
"Szukam%ls w"
"Buscando%ls en:"
"Пошук%ls в"

FindNewSearch
"&Новый поиск"
"&New search"
"&Nové hledání"
"&Neue Suche"
"&Új keresés"
"&Od nowa..."
"&Nueva búsqueda"
"&Новий пошук"

FindGoTo
"Пе&рейти"
"&Go to"
"&Jdi na"
"&Gehe zu"
"U&grás"
"&Idź do"
"&Ir a"
"Пе&рейти"

FindView
"&Просм"
"&View"
"Zo&braz"
"&Betrachten"
"Meg&néz"
"&Podgląd"
"&Ver "
"&Прогл"

FindEdit
"&Редакт"
"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
"&Редаг"

FindPanel
"Пане&ль"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Do panelu"
"&Panel"
"Пане&ль"

FindStop
"С&топ"
"&Stop"
"&Stop"
"&Stoppen"
"&Állj"
"&Stop"
"D&etener"
"С&топ"

FindDone
l:
"Поиск закончен. Найдено файлов: %d, папок: %d"
"Search done. Found files: %d, folders: %d"
"Hledání ukončeno. Nalezeno %d soubor(ů) a %d adresář(ů)"
"Suche beendet. %d Datei(en) und %d Ordner gefunden."
"A keresés kész. %d fájlt és %d mappát találtam."
"Wyszukiwanie zakończone (znalazłem %d plików i %d folderów)"
"Búsqueda finalizada. Encontrados %d archivo(s) y %d directorio(s)"
"Пошук закінчено. Знайдено файли: %d, теки: %d"

FindCancel
"Отм&ена"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"Від&міна"

FindFound
l:
" Файлов: %d, папок: %d "
" Files: %d, folders: %d "
" Souborů: %d, adresářů: %d "
" Dateien: %d, Ordner: %d "
" Fájlt: %d, mappát: %d "
" Plików: %d, folderów: %d "
" Ficheros: %d, carpetas: %d "
" Файлів: %d, тек: %d "

FindFileAdvancedTitle
l:
"Дополнительные параметры поиска"
"Find file advanced options"
"Pokročilé nastavení vyhledávání souborů"
"Erweiterte Optionen"
"Fájlkeresés haladó beállításai"
"Zaawansowane opcje wyszukiwania"
"Opciones avanzada de búsqueda de archivo"
"Додаткові параметри пошуку"

FindFileSearchFirst
"Проводить поиск в &первых:"
"Search only in the &first:"
"Hledat po&uze v prvních:"
"Nur &in den ersten x Bytes:"
"Keresés csak az első &x bájtban:"
"Szukaj wyłącznie w &pierwszych:"
"Buscar solamente en el &primer:"
"Проводити пошук у &перших:"

FindAlternateStreams
"Обрабатывать &альтернативные потоки данных"
"Process &alternate data streams"
upd:"Process &alternate data streams"
upd:"Process &alternate data streams"
"&Alternatív adatsávok (stream) feldolgozása"
upd:"Process &alternate data streams"
"Procesar flujo alternativo de datos"
"Обробляти &альтернативні потоки даних"

FindAlternateModeTypes
"&Типы колонок"
"Column &types"
"&Typ sloupců"
"Spalten&typen"
"Oszlop&típusok"
"&Typy kolumn"
"&Tipos de columna"
"&Типи колонок"

FindAlternateModeWidths
"&Ширина колонок"
"Column &widths"
"Šíř&ka sloupců"
"Spalten&breiten"
"Oszlop&szélességek"
"&Szerokości kolumn"
"Anc&ho de columna"
"&Ширина колонок"

FoldTreeSearch
l:
"Поиск:"
"Search:"
"Hledat:"
"Suchen:"
"Keresés:"
"Wyszukiwanie:"
"Buscar:"
"Пошук:"

GetCodePageTitle
l:
"Кодовые страницы"
"Code pages"
upd:"Znakové sady:"
upd:"Tabellen"
"Kódlapok"
"Strony kodowe"
"Tablas"
"Кодові сторінки"

GetCodePageSystem
"Системные"
"System"
upd:"System"
upd:"System"
"Rendszer"
upd:"System"
"Sistema"
"Системні"

GetCodePageUnicode
"Юникод"
"Unicode"
upd:"Unicode"
upd:"Unicode"
"Unicode"
upd:"Unicode"
"Unicode"
"Юнікод"

GetCodePageFavorites
"Избранные"
"Favorites"
upd:"Favorites"
upd:"Favorites"
"Kedvencek"
upd:"Favorites"
"Favoritos"
"Вибрані"

GetCodePageOther
"Прочие"
"Other"
upd:"Other"
upd:"Other"
"Egyéb"
upd:"Other"
"Otro"
"Інші"

GetCodePageBottomTitle
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Strg-H, Entf, Einf, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"

GetCodePageBottomShortTitle
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Strg-H, Entf, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"

GetCodePageEditCodePageName
"Изменить имя кодовой страницы"
"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
"Editar nombre de tabla (codepage)"
"Змінити ім'я кодової сторінки"

GetCodePageResetCodePageName
"&Сбросить"
"&Reset"
upd:"&Reset"
upd:"&Reset"
upd:"&Reset"
upd:"&Reset"
"&Reiniciar"
"&Скинути"

HighlightTitle
l:
"Раскраска файлов"
"Files highlighting"
"Zvýrazňování souborů"
"Farbmarkierungen"
"Fájlkiemelések, rendezési csoportok"
"Wyróżnianie plików"
"Resaltado de archivos"
"Розмальовка файлів"

HighlightBottom
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Nahoru,Ctrl-Dolů"
"Einf,Entf,F4,F5,StrgUp,StrgDown"
"Ins,Del,F4,F5,Ctrl-Fel,Ctrl-Le"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"

HighlightUpperSortGroup
"Верхняя группа сортировки"
"Upper sort group"
"Vzesupné řazení"
"Obere Sortiergruppen"
"Felsőbbrendű csoport"
"Górna grupa sortowania"
"Grupo de ordenamiento de arriba"
"Верхня група сортування"

HighlightLowerSortGroup
"Нижняя группа сортировки"
"Lower sort group"
"Sestupné řazení"
"Untere Sortiergruppen"
"Alsóbbrendű csoport"
"Dolna grupa sortowania"
"Grupo de ordenamiento de abajo"
"Нижня група сортування"

HighlightLastGroup
"Наименее приоритетная группа раскраски"
"Lowest priority highlighting group"
"Zvýraznění nejnižší prority"
"Farbmarkierungen mit niedrigster Priorität"
"Legalacsonyabb rendű csoport"
"Grupa wyróżniania o najniższym priorytecie"
"Resaltado de grupo con baja prioridad"
"Найменш пріоритетна група розмальовки"

HighlightAskDel
"Вы хотите удалить раскраску для"
"Do you wish to delete highlighting for"
"Přejete si smazat zvýraznění pro"
"Wollen Sie Farbmarkierungen löschen für"
"Biztosan törli a kiemelést?"
"Czy chcesz usunąć wyróżnianie dla"
"Desea borrar resaltado para"
"Ви хочете видалити розмальовку для"

HighlightWarning
"Будут потеряны все Ваши настройки"
"You will lose all changes"
"Všechny změny budou ztraceny"
"Sie verlieren jegliche Änderungen"
"Minden változtatás elvész"
"Wszystkie zmiany zostaną utracone"
"Usted perderá todos los cambios"
"Втрачені всі Ваші налаштування"

HighlightAskRestore
"Вы хотите восстановить раскраску файлов по умолчанию?"
"Do you wish to restore default highlighting?"
"Přejete si obnovit výchozí nastavení?"
"Wollen Sie Standard-Farbmarkierungen wiederherstellen?"
"Visszaállítja az alapértelmezett kiemeléseket?"
"Czy przywrócić wyróżnianie domyślne?"
"Desea restablecer resaltado por defecto?"
"Ви хочете відновити забарвлення файлів за замовчуванням?"

HighlightMarkChar
"Оп&циональный символ пометки,"
"Optional markin&g character,"
"Volitelný &znak pro označení určených souborů,"
"Optionale Markierun&g mit Zeichen,"
"Megadható &jelölő karakter"
"Opcjonalny znak &wyróżniający zaznaczone pliki,"
"Ca&racter opcional para marcar archivos específicos"
"Оп&ціональний символ позначки,"

HighlightTransparentMarkChar
"прозра&чный"
"tra&nsparent"
"průh&ledný"
"tra&nsparent"
"át&látszó"
"prze&zroczyste"
"tra&nsparente"
"проз&орий"

HighlightColors
" Цвета файлов (\"чёрный на чёрном\" - цвет по умолчанию) "
" File name colors (\"black on black\" - default color) "
" Barva názvu souborů (\"černá na černé\" - výchozí barva) "
" Dateinamenfarben (\"Schwarz auf Schwarz\"=Standard) "
" Fájlnév színek (feketén fekete = alapértelmezett szín) "
" Kolory nazw plików (domyślny - \"czarny na czarnym\") "
" Colores de archivos (\"negro en negro\" - color por defecto) "
" Кольори файлів (\"чорний на чорному\" ​​- колір за замовчуванням) "

HighlightFileName1
"&1. Обычное имя файла                "
"&1. Normal file name               "
"&1. Normální soubor            "
"&1. Normaler Dateiname             "
"&1. Normál fájlnév                  "
"&1. Nazwa pliku bez zaznaczenia    "
"&1. Normal  "
"&1. Звичайне ім'я файлу              "

HighlightFileName2
"&3. Помеченное имя файла             "
"&3. Selected file name             "
"&3. Vybraný soubor             "
"&3. Markierter Dateiame            "
"&3. Kijelölt fájlnév                "
"&3. Zaznaczenie                    "
"&3. Seleccionado"
"&3. Позначене ім'я файлу             "

HighlightFileName3
"&5. Имя файла под курсором           "
"&5. File name under cursor         "
"&5. Soubor pod kurzorem        "
"&5. Dateiname unter Cursor         "
"&5. Kurzor alatti fájlnév           "
"&5. Nazwa pliku pod kursorem       "
"&5. Bajo cursor "
"&5. Ім'я файлу під курсором          "

HighlightFileName4
"&7. Помеченное под курсором имя файла"
"&7. File name selected under cursor"
"&7. Vybraný soubor pod kurzorem"
"&7. Dateiname markiert unter Cursor"
"&7. Kurzor alatti kijelölt fájlnév  "
"&7. Zaznaczony plik pod kursorem   "
"&7. Se&leccionado bajo cursor"
"&7. Позначене під курсором ім'я файлу"

HighlightMarking1
"&2. Пометка"
"&2. Marking"
"&2. Označení"
"&2. Markierung"
"&2. Jelölő kar.:"
"&2. Zaznaczenie"
"&2. Marcado"
"&2. Помітка"

HighlightMarking2
"&4. Пометка"
"&4. Marking"
"&4. Označení"
"&4. Markierung"
"&4. Jelölő kar.:"
"&4. Zaznaczenie"
"&4. Marcado"
"&4. Помітка"

HighlightMarking3
"&6. Пометка"
"&6. Marking"
"&6. Označení"
"&6. Markierung"
"&6. Jelölő kar.:"
"&6. Zaznaczenie"
"&6. Marcado"
"&6. Помітка"

HighlightMarking4
"&8. Пометка"
"&8. Marking"
"&8. Označení"
"&8. Markierung"
"&8. Jelölő kar.:"
"&8. Zaznaczenie"
"&8. Marcado"
"&8. Помітка"

HighlightExample1
"║filename.ext │"
"║filename.ext │"
"║filename.ext │"
"║dateinam.erw │"
"║fájlneve.kit │"
"║nazwa.roz    │"
"║nombre.ext   │"
"║filename.ext │"

HighlightExample2
"║ filename.ext│"
"║ filename.ext│"
"║ filename.ext│"
"║ dateinam.erw│"
"║ fájlneve.kit│"
"║ nazwa.roz   │"
"║ nombre.ext  │"
"║filename.ext │"

HighlightContinueProcessing
"Продолжать &обработку"
"C&ontinue processing"
"Pokračovat ve zpracová&ní"
"Verarbeitung f&ortsetzen"
"Folyamatos f&eldolgozás"
"K&ontynuuj przetwarzanie"
"C&ontinuar procesando"
"Продовжувати &обробку"

InfoTitle
l:
"Информация"
"Information"
"Informace"
"Informationen"
"Információk"
"Informacja"
"Información"
"Інформация"

InfoCompName
"Имя компьютера"
"Computer name"
"Název počítače"
"Computername"
"Számítógép neve"
"Nazwa komputera"
"Nombre computadora"
"Им'я комп'ютера"

InfoUserName
"Имя пользователя"
"User name"
"Jméno uživatele"
"Benutzername"
"Felhasználói név"
"Nazwa użytkownika"
"Nombre usuario"
"Им'я користувача"

InfoDisk
"диск"
"disk"
"disk"
"Laufwerk"
"lemez"
"dysk"
"disco"
"диск"

InfoDiskTotal
"Всего байтов"
"Total bytes"
"Celkem bytů"
"Bytes gesamt"
"Összes bájt"
"Razem bajtów"
"Total de bytes"
"Всього байтів"

InfoDiskFree
"Свободных байтов"
"Free bytes"
"Volných bytů"
"Bytes frei"
"Szabad bájt"
"Wolnych bajtów"
"Bytes libres"
"Вільних байтів"

InfoDiskLabel
"Метка тома"
"Volume label"
"Popisek disku"
"Laufwerksbezeichnung"
"Kötet címke"
"Etykieta woluminu"
"Etiqueta de volumen"
"Мітка тома"

InfoDiskNumber
"Серийный номер"
"Serial number"
"Sériové číslo"
"Seriennummer"
"Sorozatszám"
"Numer seryjny"
"Número de serie"
"Серійний номер"

InfoMemory
" Память "
" Memory "
" Paměť "
" Speicher "
" Memória "
" Pamięć "
" Memoria "
" Пам'ять "

InfoMemoryLoad
"Загрузка памяти"
"Memory load"
"Zatížení paměti"
"Speicherverbrauch"
"Használt memória"
"Użycie pamięci"
"Carga en Memoria"
"Завантаження пам'яті"

InfoMemoryTotal
"Всего памяти"
"Total memory"
"Celková paměť"
"Speicher gesamt"
"Összes memória"
"Całkowita pamięć"
"Total memoria"
"Всього пам'яті"

InfoMemoryFree
"Свободно памяти"
"Free memory"
"Volná paměť"
"Speicher frei"
"Szabad memória"
"Wolna pamięć"
"Memoria libre"
"Вільно пам'яті"

InfoSharedMemory
"Разделяемая память"
"Shared memory"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
"Спільна пам'ять"

InfoBufferMemory
"Буферизованная память"
"Buffer memory"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
"Буферизована пам'ять"

InfoPageFileTotal
"Всего файла подкачки"
"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
"Archivo de paginación total"
"Всього файлу підкачки"

InfoPageFileFree
"Свободно файла подкачки"
"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
"Archivo de paginación libre"
"Вільно файлу підкачки"

InfoDizAbsent
"Файл описания папки отсутствует"
"Folder description file is absent"
"Soubor s popisem adresáře chybí"
"Keine Datei mit Ordnerbeschreibungen vorhanden."
"Mappa megjegyzésfájl nincs"
"Plik opisu katalogu nie istnieje"
"archivo descripción del directorio está ausente"
"Файл опису теки відсутній"

ScanningFolder
"Просмотр папки"
"Scanning the folder"
"Prohledávám adresář"
"Scanne den Ordner"
"Mappák olvasása..."
"Przeszukuję katalog"
"Explorando el directorio"
"Перегляд теки"

MakeFolderTitle
l:
"Создание папки"
"Make folder"
"Vytvoření adresáře"
"Ordner erstellen"
"Új mappa létrehozása"
"Utwórz katalog"
"Crear directorio"
"Створення теки"

CreateFolder
"Создать п&апку"
"Create the &folder"
"Vytvořit &adresář"
"Diesen &Ordner erstellen:"
"Mappa &neve:"
"Nazwa katalogu"
"Nombre del directorio"
"Створити т&еку"

MultiMakeDir
"Обрабатыват&ь несколько имён папок"
"Process &multiple names"
"Zpracovat &více názvů"
"&Mehrere Namen verarbeiten (getrennt durch Semikolon)"
"Töb&b név feldolgozása"
"Przetwarzaj &wiele nazw"
"Procesar &múltiples nombres"
"Оброблят&и кілька імен тек"

IncorrectDirList
"Неправильный список папок"
"Incorrect folders list"
"Neplatný seznam adresářů"
"Fehlerhafte Ordnerliste"
"Hibás mappalista"
"Błędna lista folderów"
"Listado de directorios incorrecto"
"Неправильний список тек"

CannotCreateFolder
"Ошибка создания папки"
"Cannot create the folder"
"Adresář nelze vytvořit"
"Konnte den Ordner nicht erstellen"
"A mappa nem hozható létre"
"Nie mogę utworzyć katalogu"
"No se puede crear el directorio"
"Помилка створення теки"

MenuBriefView
l:
"&Краткий                  LCtrl-1"
"&Brief              LCtrl-1"
"&Stručný                  LCtrl-1"
"&Kurz                 LStrg-1"
"&Rövid              BalCtrl-1"
"&Skrótowy             LCtrl-1"
"&Breve                 LCtrl-1"
"&Короткий                 LCtrl-1"

MenuMediumView
"&Средний                  LCtrl-2"
"&Medium             LCtrl-2"
"S&třední                  LCtrl-2"
"&Mittel               LStrg-2"
"&Közepes            BalCtrl-2"
"Ś&redni               LCtrl-2"
"&Medio                 LCtrl-2"
"&Середній                 LCtrl-2"

MenuFullView
"&Полный                   LCtrl-3"
"&Full               LCtrl-3"
"&Plný                     LCtrl-3"
"&Voll                 LStrg-3"
"&Teljes             BalCtrl-3"
"&Pełny                LCtrl-3"
"&Completo              LCtrl-3"
"&Повний                   LCtrl-3"

MenuWideView
"&Широкий                  LCtrl-4"
"&Wide               LCtrl-4"
"Š&iroký                   LCtrl-4"
"B&reitformat          LStrg-4"
"&Széles             BalCtrl-4"
"S&zeroki              LCtrl-4"
"&Amplio                LCtrl-4"
"&Широкий                  LCtrl-4"

MenuDetailedView
"&Детальный                LCtrl-5"
"Detai&led           LCtrl-5"
"Detai&lní                 LCtrl-5"
"Detai&lliert          LStrg-5"
"Rész&letes          BalCtrl-5"
"Ze sz&czegółami       LCtrl-5"
"De&tallado             LCtrl-5"
"&Детальний                LCtrl-5"

MenuDizView
"&Описания                 LCtrl-6"
"&Descriptions       LCtrl-6"
"P&opisky                  LCtrl-6"
"&Beschreibungen       LStrg-6"
"Fájl&megjegyzések   BalCtrl-6"
"&Opisy                LCtrl-6"
"&Descripción           LCtrl-6"
"&Описи                    LCtrl-6"

MenuLongDizView
"Д&линные описания         LCtrl-7"
"Lon&g descriptions  LCtrl-7"
"&Dlouhé popisky           LCtrl-7"
"Lan&ge Beschreibungen LStrg-7"
"&Hosszú megjegyzés  BalCtrl-7"
"&Długie opisy         LCtrl-7"
"Descripción lar&ga     LCtrl-7"
"Д&овгі описи              LCtrl-7"

MenuOwnersView
"Вл&адельцы файлов         LCtrl-8"
"File own&ers        LCtrl-8"
"Vlastník so&uboru         LCtrl-8"
"B&esitzer             LStrg-8"
"Fájl tula&jdonos    BalCtrl-8"
"&Właściciele          LCtrl-8"
"Du&eños de archivos    LCtrl-8"
"Власники файлів           LCtrl-8"

MenuLinksView
"Свя&зи файлов             LCtrl-9"
"File lin&ks         LCtrl-9"
"Souborové lin&ky          LCtrl-9"
"Dateilin&ks           LStrg-9"
"Fájl li&nkek        BalCtrl-9"
"Dowiąza&nia           LCtrl-9"
"En&laces               LCtrl-9"
"Зв'язки файлів            LCtrl-9"

MenuAlternativeView
"Аль&тернативный полный    LCtrl-0"
"&Alternative full   LCtrl-0"
"&Alternativní plný        LCtrl-0"
"&Alternativ voll      LStrg-0"
"&Alternatív teljes  BalCtrl-0"
"&Alternatywny         LCtrl-0"
"Alternativo com&pleto  LCtrl-0"
"Аль&тернативний повний    LCtrl-0"

MenuInfoPanel
l:
"Панель ин&формации        Ctrl-L"
"&Info panel         Ctrl-L"
"Panel In&fo               Ctrl-L"
"&Infopanel            Strg-L"
"&Info panel         Ctrl-L"
"Panel informacy&jny   Ctrl-L"
"Panel &información     Ctrl-L"
"Панель ін&формациї        Ctrl-L"

MenuTreePanel
"Де&рево папок             Ctrl-T"
"&Tree panel         Ctrl-T"
"Panel St&rom              Ctrl-T"
"Baumansich&t          Strg-T"
"&Fastruktúra        Ctrl-T"
"Drz&ewo               Ctrl-T"
"Panel árbol           Ctrl-T"
"Де&рево тек               Ctrl-T"

MenuQuickView
"Быстры&й просмотр         Ctrl-Q"
"Quick &view         Ctrl-Q"
"Z&běžné zobrazení         Ctrl-Q"
"Sc&hnellansicht       Strg-Q"
"&Gyorsnézet         Ctrl-Q"
"Sz&ybki podgląd       Ctrl-Q"
"&Vista rápida          Ctrl-Q"
"Швидки&й перегляд         Ctrl-Q"

MenuSortModes
"Режим&ы сортировки        Ctrl-F12"
"&Sort modes         Ctrl-F12"
"Módy řaze&ní              Ctrl-F12"
"&Sortiermodi          Strg-F12"
"R&endezési elv      Ctrl-F12"
"Try&by sortowania     Ctrl-F12"
"&Ordenar por...        Ctrl-F12"
"Режим&и сортування        Ctrl-F12"

MenuTogglePanel
"Панель &Вкл/Выкл          Ctrl-F1"
"Panel &On/Off       Ctrl-F1"
"Panel &Zap/Vyp            Ctrl-F1"
"&Panel ein/aus        Strg-F1"
"&Panel be/ki        Ctrl-F1"
"Włącz/Wyłącz pane&l   Ctrl-F1"
"Panel &Si/No           Ctrl-F1"
"Панель &Ввмк/Вимк         Ctrl-F1"

MenuReread
"П&еречитать               Ctrl-R"
"&Re-read            Ctrl-R"
"Obno&vit                  Ctrl-R"
"Aktualisie&ren        Strg-R"
"Friss&ítés          Ctrl-R"
"Odśw&ież              Ctrl-R"
"&Releer                Ctrl-R"
"Перечитати                Ctrl-R"

MenuChangeDrive
"С&менить диск             Alt-F1"
"&Change drive       Alt-F1"
"Z&měnit jednotku          Alt-F1"
"Laufwerk we&chseln    Alt-F1"
"Meghajtó&váltás     Alt-F1"
"Z&mień napęd          Alt-F1"
"Cambiar &unidad        Alt-F1"
"З&мінити диск             Alt-F1"

MenuView
l:
"&Просмотр              F3"
"&View               F3"
"&Zobrazit                   F3"
"&Betrachten           F3"
"&Megnéz               F3"
"&Podgląd                   F3"
"&Ver                   F3"
"&Перегляд              F3"

MenuEdit
"&Редактирование        F4"
"&Edit               F4"
"&Editovat                   F4"
"B&earbeiten           F4"
"&Szerkeszt            F4"
"&Edytuj                    F4"
"&Editar                F4"
"&Редагування           F4"

MenuCopy
"&Копирование           F5"
"&Copy               F5"
"&Kopírovat                  F5"
"&Kopieren             F5"
"Más&ol                F5"
"&Kopiuj                    F5"
"&Copiar                F5"
"&Копіювання            F5"

MenuMove
"П&еренос               F6"
"&Rename or move     F6"
"&Přejmenovat/Přesunout      F6"
"Ve&rschieben/Umben.   F6"
"Át&nevez-Mozgat       F6"
"&Zmień nazwę lub przenieś  F6"
"&Renombrar o mover     F6"
"П&еренесення           F6"

MenuCreateFolder
"&Создание папки        F7"
"&Make folder        F7"
"&Vytvořit adresář           F7"
"&Ordner erstellen     F7"
"Ú&j mappa             F7"
"U&twórz katalog            F7"
"Crear &directorio      F7"
"&Створення теки        F7"

MenuDelete
"&Удаление              F8"
"&Delete             F8"
"&Smazat                     F8"
"&Löschen              F8"
"&Töröl                F8"
"&Usuń                      F8"
"&Borrar                F8"
"&Видалення             F8"

MenuWipe
"Уни&чтожение           Alt-Del"
"&Wipe               Alt-Del"
"&Vymazat                    Alt-Del"
"&Sicher löschen       Alt-Entf"
"&Kisöpör              Alt-Del"
"&Wymaż                     Alt-Del"
"&Eliminar              Alt-Del"
"Зни&щення              Alt-Del"

MenuAdd
"&Архивировать          Shift-F1"
"Add &to archive     Shift-F1"
"Přidat do &archívu          Shift-F1"
"Zu Archiv &hinzuf.    Umsch-F1"
"Tömörhöz ho&zzáad     Shift-F1"
"&Dodaj do archiwum         Shift-F1"
"Agregar a arc&hivo     Shift-F1"
"&Архівувати            Shift-F1"

MenuExtract
"Распако&вать           Shift-F2"
"E&xtract files      Shift-F2"
"&Rozbalit soubory           Shift-F2"
"Archiv e&xtrahieren   Umsch-F2"
"Tömörből ki&bont      Shift-F2"
"&Rozpakuj archiwum         Shift-F2"
"E&xtraer archivos      Shift-F2"
"Розпаку&вати           Shift-F2"

MenuArchiveCommands
"Архивн&ые команды      Shift-F3"
"Arc&hive commands   Shift-F3"
"Příkazy arc&hívu            Shift-F3"
"Arc&hivbefehle        Umsch-F3"
"Tömörítő &parancsok   Shift-F3"
"Po&lecenie archiwizera     Shift-F3"
"Co&mandos archivo      Shift-F3"
"Архівн&і команди       Shift-F3"

MenuAttributes
"А&трибуты файлов       Ctrl-A"
"File &attributes    Ctrl-A"
"A&tributy souboru           Ctrl-A"
"Datei&attribute       Strg-A"
"Fájl &attribútumok    Ctrl-A"
"&Atrybuty pliku            Ctrl-A"
"Cambiar &atributos     Ctrl-A"
"А&трибути файлів       Ctrl-A"

MenuApplyCommand
"Применить коман&ду     Ctrl-G"
"A&pply command      Ctrl-G"
"Ap&likovat příkaz           Ctrl-G"
"Befehl an&wenden      Strg-G"
"Parancs &végrehajtása Ctrl-G"
"Zastosuj pole&cenie        Ctrl-G"
"A&plicar comando       Ctrl-G"
"Примінити коман&ду     Ctrl-G"

MenuDescribe
"&Описание файлов       Ctrl-Z"
"Descri&be files     Ctrl-Z"
"Přidat popisek sou&borům    Ctrl-Z"
"Beschrei&bung ändern  Strg-Z"
"Fájlmegje&gyzés       Ctrl-Z"
"&Opisz pliki               Ctrl-Z"
"Describir &archivo     Ctrl-Z"
"&Опис файлів           Ctrl-Z"

MenuSelectGroup
"Пометить &группу       Gray +"
"Select &group       Gray +"
"Oz&načit skupinu            Num +"
"&Gruppe auswählen     Num +"
"Csoport k&ijelölése   Szürke +"
"Zaznacz &grupę             Szary +"
"Seleccionar &grupo     Gray +"
"Позначити &групу       Gray +"

MenuUnselectGroup
"С&нять пометку         Gray -"
"U&nselect group     Gray -"
"O&dznačit skupinu           Num -"
"G&ruppe abwählen      Num -"
"Jelölést l&evesz      Szürke -"
"Odz&nacz grupę             Szary -"
"Deseleccio&nar grupo   Gray -"
"З&няти позначку         Gray -"

MenuInvertSelection
"&Инверсия пометки      Gray *"
"&Invert selection   Gray *"
"&Invertovat výběr           Num *"
"Auswah&l umkehren     Num *"
"Jelölést meg&fordít   Szürke *"
"Od&wróć zaznaczenie        Szary *"
"&Invertir selección    Gray *"
"&Інверсія позначки     Gray *"

MenuRestoreSelection
"Восстановить по&метку  Ctrl-M"
"Re&store selection  Ctrl-M"
"&Obnovit výběr              Ctrl-M"
"Auswahl wiederher&st. Strg-M"
"Jel&ölést visszatesz  Ctrl-M"
"Odtwórz zaznaczen&ie       Ctrl-M"
"Re&staurar selec.      Ctrl-M"
"Відновити по&значку    Ctrl-M"

MenuFindFile
l:
"&Поиск файла              Alt-F7"
"&Find file           Alt-F7"
"H&ledat soubor                  Alt-F7"
"Dateien &finden       Alt-F7"
"Fájl&keresés         Alt-F7"
"&Znajdź plik               Alt-F7"
"Buscar &archivos       Alt-F7"
"&Пошук файла              Alt-F7"

MenuHistory
"&История команд           Alt-F8"
"&History             Alt-F8"
"&Historie                       Alt-F8"
"&Historie             Alt-F8"
"Parancs &előzmények  Alt-F8"
"&Historia                  Alt-F8"
"&Historial             Alt-F8"
"&Історія команд           Alt-F8"

MenuVideoMode
"Видео&режим               Alt-F9"
"&Video mode          Alt-F9"
"&Video mód                      Alt-F9"
"Ansicht<->&Vollbild   Alt-F9"
"&Video mód           Alt-F9"
"&Tryb wyświetlania         Alt-F9"
"Modo de video         Alt-F9"
"Відео&режим               Alt-F9"

MenuFindFolder
"Поис&к папки              Alt-F10"
"Fi&nd folder         Alt-F10"
"Hl&edat adresář                 Alt-F10"
"Ordner fi&nden        Alt-F10"
"&Mappakeresés        Alt-F10"
"Znajdź kata&log            Alt-F10"
"Buscar &directorios    Alt-F10"
"Пошу&к теки               Alt-F10"

MenuViewHistory
"Ис&тория просмотра        Alt-F11"
"File vie&w history   Alt-F11"
"Historie &zobrazení souborů     Alt-F11"
"Be&trachterhistorie   Alt-F11"
"Fáj&l előzmények     Alt-F11"
"Historia &podglądu plików  Alt-F11"
"Historial &visor       Alt-F11"
"Іс&торія перегляду        Alt-F11"

MenuFoldersHistory
"Ист&ория папок            Alt-F12"
"F&olders history     Alt-F12"
"Historie &adresářů              Alt-F12"
"&Ordnerhistorie       Alt-F12"
"Ma&ppa előzmények    Alt-F12"
"Historia &katalogów        Alt-F12"
"Histo&rial dir.        Alt-F12"
"Іст&орія тек              Alt-F12"

MenuSwapPanels
"По&менять панели          Ctrl-U"
"&Swap panels         Ctrl-U"
"Prohodit panel&y                Ctrl-U"
"Panels tau&schen      Strg-U"
"Panel&csere          Ctrl-U"
"Z&amień panele             Ctrl-U"
"I&nvertir paneles      Ctrl-U"
"Зм&інити панелі            Ctrl-U"

MenuTogglePanels
"Панели &Вкл/Выкл          Ctrl-O"
"&Panels On/Off       Ctrl-O"
"&Panely Zap/Vyp                 Ctrl-O"
"&Panels ein/aus       Strg-O"
"Panelek &be/ki       Ctrl-O"
"&Włącz/Wyłącz panele       Ctrl-O"
"&Paneles Si/No         Ctrl-O"
"Панели &Ввмк/Вимк          Ctrl-O"

MenuCompareFolders
"&Сравнение папок"
"&Compare folders"
"Po&rovnat adresáře"
"Ordner verglei&chen"
"Mappák össze&hasonlítása"
"Porówna&j katalogi"
"&Compara directorios"
"&Порівняння тек"

MenuUserMenu
"Меню пользовател&я"
"Edit user &menu"
"Upravit uživatelské &menu"
"Benutzer&menu editieren"
"Felhasználói m&enü szerk."
"Edytuj &menu użytkownika"
"Editar &menú usuario"
"Меню користувач&а"

MenuFileAssociations
"&Ассоциации файлов"
"File &associations"
"Asocia&ce souborů"
"Dat&eiverknüpfungen"
"Fájl&társítások"
"Prz&ypisania plików"
"&Asociar archivos"
"&Ассоциації файлів"

MenuBookmarks
"Зак&ладки на папки"
"Fol&der bookmarks"
"A&dresářové zkratky"
"Or&dnerschnellzugriff"
"Mappa gyorsbillent&yűk"
"&Skróty katalogów"
"Acc&eso a directorio"
"Зак&ладки на теки"

MenuFilter
"&Фильтр панели файлов     Ctrl-I"
"File panel f&ilter   Ctrl-I"
"F&iltr panelu souborů           Ctrl-I"
"Panelf&ilter          Strg-I"
"Fájlpanel &szűrők    Ctrl-I"
"&Filtr panelu plików       Ctrl-I"
"F&iltro de paneles     Ctrl-I"
"&Фільтр панелі файлів      Ctrl-I"

MenuPluginCommands
"Команды внешних мо&дулей  F11"
"Pl&ugin commands     F11"
"Příkazy plu&ginů                F11"
"Pl&uginbefehle        F11"
"Pl&ugin parancsok    F11"
"Pl&uginy                   F11"
"Comandos de pl&ugin    F11"
"Команди зовнішніх мо&дулів F11"

MenuWindowsList
"Список экра&нов           F12"
"Sc&reens list        F12"
"Seznam obrazove&k               F12"
"Seite&nliste          F12"
"Képer&nyők           F12"
"L&ista ekranów             F12"
"&Listado ventanas      F12"
"Список екра&нів           F12"

MenuProcessList
"Список &задач             Ctrl-W"
"Task &list           Ctrl-W"
"Seznam úl&oh                    Ctrl-W"
"Task&liste            Strg-W"
"Futó p&rogramok      Ctrl-W"
"Lista za&dań               Ctrl-W"
"Lista de &tareas       Ctrl-W"
"Список & завдань          Ctrl-W"

MenuHotPlugList
"Список Hotplug-&устройств"
"Ho&tplug devices list"
"Seznam v&yjímatelných zařízení"
"Sicheres En&tfernen"
"H&otplug eszközök"
"Lista urządzeń Ho&tplug"
"Lista de dispositivos ho&tplug"
"Список Hotplug-&пристроїв"

MenuSystemSettings
l:
"Систе&мные параметры"
"S&ystem settings"
"Nastavení S&ystému"
"&Grundeinstellungen"
"&Rendszer beállítások"
"Ustawienia &systemowe"
"&Sistema      "
"Систе&мні параметри"

MenuPanelSettings
"Настройки па&нели"
"&Panel settings"
"Nastavení &Panelů"
"&Panels einrichten"
"&Panel beállítások"
"Ustawienia &panelu"
"&Paneles      "
"Налаштування па&нелі"

MenuInterface
"Настройки &интерфейса"
"&Interface settings"
"Nastavení Ro&zhraní"
"Oberfläche einr&ichten"
"Kezelő&felület beállítások"
"Ustawienia &interfejsu"
"&Interfaz     "
"Налаштування &інтерфейса"

MenuLanguages
"&Языки"
"Lan&guages"
"Nastavení &Jazyka"
"Sprac&hen"
"N&yelvek (Languages)"
"&Język"
"&Idiomas"
"&Мови"

MenuInput
"Параметры &ввода"
"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Параметры &ввода"

MenuPluginsConfig
"Параметры &внешних модулей"
"Pl&ugins configuration"
"Nastavení Plu&ginů"
"Konfiguration von Pl&ugins"
"Pl&ugin beállítások"
"Konfiguracja p&luginów"
"Configuración de pl&ugins"
"Параметри &зовнішніх модулів"

MenuPluginsManagerSettings
"Параметры менеджера внешних модулей"
"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
"Параметри менеджера зовнішніх модулів"

MenuDialogSettings
"Настройки &диалогов"
"Di&alog settings"
"Nastavení Dialo&gů"
"Di&aloge einrichten"
"Pár&beszédablak beállítások"
"Ustawienia okna &dialogowego"
"Opciones de di&álogo"
"Налаштування &діалогів"

MenuVMenuSettings
"Настройки меню"
"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
"Налаштування меню"

MenuCmdlineSettings
"Настройки &командной строки"
"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
"Opciones de línea de comando"
"Налаштування &командного рядка"

MenuAutoCompleteSettings
"На&стройки автозавершения"
"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
"Opciones de autocompletar"
"На&лаштування автозавершення"

MenuInfoPanelSettings
"Нас&тройки информационной панели"
"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
"Opciones de panel de inf&ormación"
"Нал&аштування інформаційної панелі"

MenuConfirmation
"&Подтверждения"
"Co&nfirmations"
"P&otvrzení"
"Bestätigu&ngen"
"Meg&erősítések"
"P&otwierdzenia"
"Co&nfirmaciones"
"&Підтвердження"

MenuFilePanelModes
"Режим&ы панели файлов"
"File panel &modes"
"&Módy souborových panelů"
"Anzeige&modi von Dateipanels"
"Fájlpanel mód&ok"
"&Tryby wyświetlania panelu plików"
"&Modo de paneles de archivos"
"Режим&и панелі файлів"

MenuFileDescriptions
"&Описания файлов"
"File &descriptions"
"Popi&sy souborů"
"&Dateibeschreibungen"
"Fájl &megjegyzésfájlok"
"Opis&y plików"
"&Descripción de archivos"
"&Описи файлів"

MenuFolderInfoFiles
"Файлы описания п&апок"
"&Folder description files"
"Soubory popisů &adresářů"
"O&rdnerbeschreibungen"
"M&appa megjegyzésfájlok"
"Pliki opisu &katalogu"
"&Archivo de descripción de directorios"
"Файли описів т&ек"

MenuViewer
"Настройки про&граммы просмотра"
"&Viewer settings"
"Nastavení P&rohlížeče"
"Be&trachter einrichten"
"&Nézőke beállítások"
"Ustawienia pod&glądu"
"&Visor "
"Налаштування про&грами перегляду"

MenuEditor
"Настройки &редактора"
"&Editor settings"
"Nastavení &Editoru"
"&Editor einrichten"
"&Szerkesztő beállítások"
"Ustawienia &edytora"
"&Editor "
"Налаштування &редактора"

MenuNotifications
"Настройки &уведомлений"
"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
"Налаштування &повідомлень"

MenuCodePages
"Кодов&ые страницы"
upd:"&Code pages"
upd:"Znakové sady:"
upd:"Tabellen"
upd:"Kódlapok"
upd:"Strony kodowe"
"Tablas (code pages)"
"Кодов&і сторінки"

MenuColors
"&Цвета"
"Co&lors"
"&Barvy"
"&Farben"
"S&zínek"
"Kolo&ry"
"&Colores"
"&Кольори"

MenuFilesHighlighting
"Раскраска &файлов и группы сортировки"
"Files &highlighting and sort groups"
"Z&výrazňování souborů a skupiny řazení"
"Farbmar&kierungen und Sortiergruppen"
"Fájlkiemelések, rendezési &csoportok"
"&Wyróżnianie plików"
"&Resaltar archivos y ordenar grupos"
"Розмальовка &файлів та групи сортування"

MenuSaveSetup
"&Сохранить параметры                  Shift-F9"
"&Save setup                         Shift-F9"
"&Uložit nastavení                      Shift-F9"
"Setup &speichern                     Umsch-F9"
"Beállítások men&tése                 Shift-F9"
"&Zapisz ustawienia       Shift-F9"
"&Guardar configuración     Shift-F9"
"&Зберегти параметри                   Shift-F9"

MenuTogglePanelRight
"Панель &Вкл/Выкл          Ctrl-F2"
"Panel &On/Off       Ctrl-F2"
"Panel &Zap/Vyp            Ctrl-F2"
"Panel &ein/aus        Strg-F2"
"Panel be/&ki        Ctrl-F2"
"Włącz/wyłącz pane&l   Ctrl-F2"
"Panel &Si/No           Ctrl-F2"
"Панель &Ввмк/Вимк         Ctrl-F2"

MenuChangeDriveRight
"С&менить диск             Alt-F2"
"&Change drive       Alt-F2"
"Z&měnit jednotku          Alt-F2"
"Laufwerk &wechseln    Alt-F2"
"Meghajtó&váltás     Alt-F2"
"Z&mień napęd          Alt-F2"
"Cambiar &unidad        Alt-F2"
"З&мінити диск             Alt-F2"

MenuLeftTitle
l:
"&Левая"
"&Left"
"&Levý"
"&Links"
"&Bal"
"&Lewy"
"&Izquierdo"
"&Ліва"

MenuFilesTitle
"&Файлы"
"&Files"
"&Soubory"
"&Dateien"
"&Fájlok"
"Pl&iki"
"&Archivo"
"&Файли"

MenuCommandsTitle
"&Команды"
"&Commands"
"Pří&kazy"
"&Befehle"
"&Parancsok"
"Pol&ecenia"
"&Comandos"
"&Команди"

MenuOptionsTitle
"Па&раметры"
"&Options"
"&Nastavení"
"&Optionen"
"B&eállítások"
"&Opcje"
"&Opciones"
"Па&раметри"

MenuRightTitle
"&Правая"
"&Right"
"&Pravý"
"&Rechts"
"&Jobb"
"&Prawy"
"&Derecho"
"&Права"

MenuSortTitle
l:
"Критерий сортировки"
"Sort by"
"Seřadit podle"
"Sortieren nach"
"Rendezési elv"
"Sortuj według..."
"Ordenar por"
"Критерій сортування"

MenuSortByName
"&Имя                              Ctrl-F3"
"&Name                   Ctrl-F3"
"&Názvu                     Ctrl-F3"
"&Name                   Strg-F3"
"&Név                  Ctrl-F3"
"&nazwy                       Ctrl-F3"
"&Nombre               Ctrl-F3"
"&ІИм'я                            Ctrl-F3"

MenuSortByExt
"&Расширение                       Ctrl-F4"
"E&xtension              Ctrl-F4"
"&Přípony                   Ctrl-F4"
"&Erweiterung            Strg-F4"
"Ki&terjesztés         Ctrl-F4"
"ro&zszerzenia                Ctrl-F4"
"E&xtensión            Ctrl-F4"
"&Розширення                       Ctrl-F4"

MenuSortByWrite
"Время &записи                     Ctrl-F5"
"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
"Fecha &modificación   Ctrl-F5"
"Час &запису                       Ctrl-F5"

MenuSortBySize
"Р&азмер                           Ctrl-F6"
"&Size                   Ctrl-F6"
"&Velikosti                 Ctrl-F6"
"&Größe                  Strg-F6"
"&Méret                Ctrl-F6"
"&rozmiaru                    Ctrl-F6"
"&Tamaño               Ctrl-F6"
"Р&озмір                           Ctrl-F6"

MenuUnsorted
"&Не сортировать                   Ctrl-F7"
"&Unsorted               Ctrl-F7"
"N&eřadit                   Ctrl-F7"
"&Unsortiert             Strg-F7"
"&Rendezetlen          Ctrl-F7"
"&bez sortowania              Ctrl-F7"
"&Sin ordenar          Ctrl-F7"
"&Не сортувати                     Ctrl-F7"

MenuSortByCreation
"Время &создания                   Ctrl-F8"
"&Creation time          Ctrl-F8"
"&Data vytvoření            Ctrl-F8"
"E&rstelldatum           Strg-F8"
"Ke&letkezés ideje     Ctrl-F8"
"czasu u&tworzenia            Ctrl-F8"
"Fecha de &creación    Ctrl-F8"
"Час &створення                    Ctrl-F8"

MenuSortByAccess
"Время &доступа                    Ctrl-F9"
"&Access time            Ctrl-F9"
"Ča&su přístupu             Ctrl-F9"
"&Zugriffsdatum          Strg-F9"
"&Hozzáférés ideje     Ctrl-F9"
"czasu &użycia                Ctrl-F9"
"Fecha de &acceso      Ctrl-F9"
"Час &доступу                      Ctrl-F9"

MenuSortByChange
"Время из&менения"
"Chan&ge time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
"Час з&міни"

MenuSortByDiz
"&Описания                         Ctrl-F10"
"&Descriptions           Ctrl-F10"
"P&opisků                   Ctrl-F10"
"&Beschreibungen         Strg-F10"
"Megjegyzé&sek         Ctrl-F10"
"&opisu                       Ctrl-F10"
"&Descripciones        Ctrl-F10"
"&Опис                             Ctrl-F10"

MenuSortByOwner
"&Владельцы файлов                 Ctrl-F11"
"&Owner                  Ctrl-F11"
"V&lastníka                 Ctrl-F11"
"Bes&itzer               Strg-F11"
"Tula&jdonos           Ctrl-F11"
"&właściciela                 Ctrl-F11"
"Dueñ&o                Ctrl-F11"
"&Власники файлів                  Ctrl-F11"

MenuSortByPhysicalSize
"&Физический размер"
"&Physical size"
upd:"&Komprimované velikosti"
upd:"Kom&primierte Größe"
upd:"Tömörített mér&et"
upd:"rozmiaru po &kompresji"
upd:"Tamaño de com&presin"
"&Фізичний розмір"

MenuSortByNumLinks
"Ко&личество ссылок"
"Number of &hard links"
"Poč&tu pevných linků"
"Anzahl an &Links"
"Hardlinkek s&záma"
"&liczby dowiązań"
"Número de enlaces &rígidos"
"Кі&лькість посилань"

MenuSortByFullName
"&Полное имя"
"&Full name"
upd:"&Full name"
upd:"&Full name"
upd:"&Full name"
upd:"&Full name"
"Nombre completo"
"&Повне ім'я"

MenuSortByCustomData
upd:"Cus&tom data"
"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
"Datos opcionales"
upd:"Cus&tom data"

MenuSortUseGroups
"Использовать &группы сортировки   Shift-F11"
"Use sort &groups        Shift-F11"
"Řazení podle skup&in       Shift-F11"
"Sortier&gruppen verw.   Umsch-F11"
"Rend. cs&oport haszn. Shift-F11"
"użyj &grup sortowania        Shift-F11"
"Usar orden/&grupo      Shift-F11"
"Використовувати &групи сортування   Shift-F11"

MenuSortSelectedFirst
"Помеченные &файлы вперёд          Shift-F12"
"Show selected f&irst    Shift-F12"
"Nejdřív zobrazit vy&brané  Shift-F12"
"&Ausgewählte zuerst     Umsch-F12"
"Kijel&ölteket előre   Shift-F12"
"zazna&czone najpierw         Shift-F12"
"Mostrar seleccionados primero Shift-F12"
"Позначені &файли вперед Shift-F12"

MenuSortDirectoriesFirst
"&Каталоги вперёд"
"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
"Mostrar directorios primero"
"&Каталоги вперед"

MenuSortUseNumeric
"&Числовая сортировка"
"Num&eric sort"
"Použít čí&selné řazení"
"Nu&merische Sortierung"
"N&umerikus rendezés"
"Sortuj num&erycznie"
"Usar orden num&érico"
"&Числове сортування"

MenuSortUseCaseSensitive
"Сортировка с учётом регистра"
"Use case sensitive sort"
"Použít řazení citlivé na velikost písmen"
"Sortierung abhängig von Groß-/Kleinschreibung"
"Nagy/kisbetű érzékeny rendezés"
"Sortuj uwzględniając wielkość liter"
"Usar orden sensible a min/mayúsc."
"Сортування з урахуванням регістру"

ChangeDriveTitle
l:
"Перейти"
"Location"
"Jednotka"
"Laufwerke"
"Meghajtók"
"Napęd"
"Unidad"
"Перейти"

ChangeDriveConfigure
"Настройки меню перехода"
"Location Menu Options"
upd:"Location Menu Options"
upd:"Location Menu Options"
upd:"Location Menu Options"
upd:"Location Menu Options"
"Cambiar opciones de menú de unidades"
"Налаштування меню вибору диска"

ChangeDriveShowDiskType
"Показывать &тип диска"
"Show disk &type"
upd:"Show disk type"
upd:"Show disk type"
upd:"Show disk type"
upd:"Show disk type"
"Mostrar &tipo de disco"
"Показувати &тип диска"

ChangeDriveShowNetworkName
"Показывать &сетевое имя/путь SUBST/имя VHD"
"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
"Показувати мережеве ім'я/шлях SUBST/ім'я VHD"

ChangeDriveShowLabel
"Показывать &метку диска"
"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
"Mostrar etiqueta"
"Показувати &мітку диска"

ChangeDriveShowFileSystem
"Показывать тип &файловой системы"
"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
"Mostrar sistema de archivos"
"Показувати тип &файлової системи"

ChangeDriveShowSize
"Показывать &размер"
"Show &size"
upd:"Show &size"
upd:"Show &size"
upd:"Show &size"
upd:"Show &size"
"Mostrar tamaño"
"Показувати &розмір"

ChangeDriveShowSizeFloat
"Показывать ра&змер в стиле Windows Explorer"
"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
"Mostrar tamaño estilo &Windows Explorer"
"Показувати ро&змір у стилі Windows Explorer"

ChangeDriveShowPlugins
"Показывать &плагины"
"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
"Mostrar &plugins"
"Показувати &Плагіни"

ChangeDriveShowShortcuts
"Показывать &закладки"
"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Mostrar &bookmarks"
"Показувати &закладки"

ChangeDriveShowMounts
"Показывать точки &монтирования"
"Show &mountpoints"
upd:"Show &mountpoints"
upd:"Show &mountpoints"
upd:"Show &mountpoints"
upd:"Show &mountpoints"
upd:"Show &mountpoints"
upd:"Show &mountpoints"

ChangeDriveShowNetworkDrive
"Показывать параметры се&тевых дисков"
"Show n&etwork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
"Mostrar parámetros unidades de red"
"Показувати параметри ме&режевих дисків"

ChangeDriveMenuFooter
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"

ChangeDriveExceptions
"Исключения:    "
upd:"Exceptions:   "
upd:"Exceptions:   "
upd:"Exceptions:   "
upd:"Exceptions:   "
upd:"Exceptions:   "
upd:"Exceptions:   "
upd:"Exceptions:   "

ChangeDriveColumn2
"Вторая колонка:"
upd:"Second column:"
upd:"Second column:"
upd:"Second column:"
upd:"Second column:"
upd:"Second column:"
upd:"Second column:"
upd:"Second column:"

ChangeDriveColumn3
"Третья колонка:"
upd:"Third column: "
upd:"Third column: "
upd:"Third column: "
upd:"Third column: "
upd:"Third column: "
upd:"Third column: "
upd:"Third column: "

EditControlHistoryFooter
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"

EditControlHistoryFooterNoDel
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"

HistoryFooter
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"

SearchFileTitle
l:
" Поиск "
" Search "
" Hledat "
" Suchen "
" Keresés "
" Szukaj "
" Buscar "
" Пошук "

CannotCreateListFile
"Ошибка создания списка файлов"
"Cannot create list file"
"Nelze vytvořit soubor se seznamem"
"Dateiliste konnte nicht erstellt werden"
"A listafájl nem hozható létre"
"Nie mogę utworzyć listy plików"
"No se puede crear archivo de lista"
"Помилка створення списку файлів"

CannotCreateListTemp
"(невозможно создать временный файл для списка)"
"(cannot create temporary file for list)"
"(nemohu vytvořit dočasný soubor pro seznam)"
"(Fehler beim Anlegen einer temporären Datei für Liste)"
"(a lista átmeneti fájl nem hozható létre)"
"(nie można utworzyć pliku tymczasowego dla listy)"
"(no se puede crear archivo temporal para lista)"
"(неможливо створити тимчасовий файл для списку)"

CannotCreateListWrite
"(невозможно записать данные в файл)"
"(cannot write data in file)"
"(nemohu zapsat data do souboru)"
"(Fehler beim Schreiben der Daten)"
"(a fájlba nem írható adat)"
"(nie można zapisać danych do pliku)"
"(no se puede escribir datos en el archivo)"
"(неможливо записати дані у файл)"

DragFiles
l:
"%d файлов"
"%d files"
"%d souborů"
"%d Dateien"
"%d fájl"
"%d plików"
"%d archivos"
"%d файлів"

DragMove
"Перенос %ls"
"Move %ls"
"Přesunout %ls"
"Verschiebe %ls"
"%ls mozgatása"
"Przenieś %ls"
"Mover %ls"
"Перенесення %ls"

DragCopy
"Копирование %ls"
"Copy %ls"
"Kopírovat %ls"
"Kopiere %ls"
"%ls másolása"
"Kopiuj %ls"
"Copiar %ls"
"Копіювання %ls"

ProcessListTitle
l:
"Список задач"
"Task list"
"Seznam úloh"
"Taskliste"
"Futó programok"
"Lista zadań"
"Lista de tareas"
"Список завдань"

ProcessListBottom
"Редактирование: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Tasten: Entf,StrgR"
"Szerk.: Del,Ctrl-R"
"Edycja: Del,Ctrl-R"
"Editar: Del,Ctrl-R"
"Редагування: Del,Ctrl-R"

QuickViewTitle
l:
"Быстрый просмотр"
"Quick view"
"Zběžné zobrazení"
"Schnellansicht"
"Gyorsnézet"
"Szybki podgląd"
"Vista rápida"
"Швидкий перегляд"

QuickViewFolder
"Папка"
"Folder"
"Adresář"
"Verzeichnis"
"Mappa"
"Folder"
"Directorio"
"Тека"

QuickViewJunction
"Связь"
"Junction"
"Křížení"
"Knotenpunkt"
"Csomópont"
"Powiązanie"
"Juntar"
"Зв'язок"

QuickViewSymlink
"Ссылка"
"Symlink"
"Symbolický link"
"Symlink"
"Szimlink"
"Link"
"Enlace"
"Посилання"

QuickViewVolMount
"Том"
"Volume"
"Svazek"
"Datenträger"
"Kötet"
"Napęd"
"Volumen"
"Том"

QuickViewContains
"Содержит:"
"Contains:"
"Obsah:"
"Enthält:"
"Tartalma:"
"Zawiera:"
"Contiene:"
"Утримує:"

QuickViewFolders
"Папок               "
"Folders          "
"Adresáře           "
"Ordner           "
"Mappák száma     "
"Katalogi            "
"Directorios      "
"Тек                 "

QuickViewFiles
"Файлов              "
"Files            "
"Soubory            "
"Dateien          "
"Fájlok száma     "
"Pliki               "
"archivos         "
"Файлів              "

QuickViewBytes
"Размер файлов       "
"Files size       "
"Velikost souborů   "
"Gesamtgröße      "
"Fájlok mérete    "
"Rozmiar plików      "
"Tamaño archivos  "
"Розмір файлів       "

QuickViewPhysical
"Физичеcкий размер  "
"Physical size    "
upd:"Komprim. velikost  "
upd:"Komprimiert      "
upd:"Tömörített méret "
upd:"Po kompresji        "
upd:"Tamaño comprimido"
"Фізичний розмір  "

QuickViewRatio
"Степень сжатия      "
"Ratio            "
"Poměr              "
"Rate             "
"Tömörítés aránya "
"Procent             "
"Promedio"
"Ступінь стиснення   "

QuickViewCluster
"Размер кластера     "
"Cluster size     "
"Velikost clusteru  "
"Clustergröße     "
"Klaszterméret    "
"Rozmiar klastra     "
"Tamaño cluster   "
"Розмір кластера     "

SetAttrTitle
l:
"Атрибуты"
"Attributes"
"Atributy"
"Attribute"
"Attribútumok"
"Atrybuty"
"Atributos"
"Атрибути"

SetAttrFor
"Изменить файловые атрибуты"
"Change file attributes for"
"Změna atributů souboru pro"
"Ändere Dateiattribute für"
"Attribútumok megváltoztatása"
"Zmień atrybuty dla"
"Cambiar atributos del archivo"
"Змінити файлові атрибути"

SetAttrSelectedObjects
"выбранных объектов"
"selected objects"
"vybrané objekty"
"markierte Objekte"
"a kijelölt objektumokon"
"wybranych obiektów"
"objetos seleccionados"
"вибраних об'єктів"

SetAttrSubfolders
"Обрабатывать &вложенные папки"
"Process sub&folders"
"Zpracovat i po&dadresáře"
"Unterordner miteinbe&ziehen"
"Az almappákon is"
"Przetwarzaj &podkatalogi"
"Procesar sub&directorios"
"Обробляти &вкладені теки"

SetAttrImmutable
"Не&изменяемый"
"Im&mutable"
upd:"Im&mutable"
upd:"Im&mutable"
upd:"Im&mutable"
upd:"Im&mutable"
upd:"Im&mutable"
upd:"Im&mutable"

SetAttrAppend
"Дополнение"
"&Append"
upd:"&Append"
upd:"&Append"
upd:"&Append"
upd:"&Append"
upd:"&Append"
upd:"&Append"

SetAttrHidden
"С&крытый"
"&Hidden"
upd:"&Hidden"
upd:"&Hidden"
upd:"&Hidden"
upd:"&Hidden"
upd:"&Hidden"
upd:"&Hidden"

SetAttrSUID
"S&UID"
"S&UID"
"S&UID"
"S&UID"
"S&UID"
"S&UID"
"S&UID"
"S&UID"

SetAttrSGID
"SGI&D"
"SGI&D"
"SGI&D"
"SGI&D"
"SGI&D"
"SGI&D"
"SGI&D"
"SGI&D"

SetAttrSticky
"Stick&y"
"Stick&y"
"Stick&y"
"Stick&y"
"Stick&y"
"Stick&y"
"Stick&y"
"Stick&y"

SetAttrOwner
"Владелец:"
"&Owner:"
"Vlastník:"
"Besitzer:"
"Tulajdonos:"
"Właściciel:"
"Dueño:"
"Власник:"

SetAttrOwnerMultiple
"(несколько значений)"
"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
"(valores múltiples)"
"(кілька значень)"

SetAttrOriginal
"Исход&ное"
"&Original"
"&Originál"
"&Original"
"&Eredeti"
"Wstaw &oryginalny"
"Ori&ginal"
"Вихід&не"

SetAttrCurrent
"Те&кущее"
"Curre&nt"
"So&učasný"
"Akt&uell"
"Aktuál&is"
"Wstaw &bieżący"
"Ac&tual"
"По&точне"

SetAttrBlank
"Сбр&ос"
"&Blank"
"P&rázdný"
"L&eer"
"&Üres"
"&Wyczyść"
"&Vaciar"
"Ски&дання"

SetAttrSet
"Установить"
"Set"
"Nastavit"
"Setzen"
"Alkalmaz"
"Usta&w"
"Poner"
"Встановити"

SetAttrTimeTitle1
l:
"ММ%cДД%cГГГГГ чч%cмм%cсс%cмс"
"MM%cDD%cYYYYY hh%cmm%css%cms"
upd:"MM%cDD%cRRRRR hh%cmm%css%cms"
upd:"MM%cTT%cJJJJJ hh%cmm%css%cms"
upd:"HH%cNN%cÉÉÉÉÉ óó%cpp%cmm%cms"
upd:"MM%cDD%cRRRRR gg%cmm%css%cms"
"MM%cDD%cAAAAA hh%cmm%css"
"ММ%cДД%cРРРРР гг%cхх%cсс%cмс"

SetAttrTimeTitle2
"ДД%cММ%cГГГГГ чч%cмм%cсс%cмс"
"DD%cMM%cYYYYY hh%cmm%css%cms"
upd:"DD%cMM%cRRRRR hh%cmm%css%cms"
upd:"TT%cMM%cJJJJJ hh%cmm%css%cms"
upd:"NN%cHH%cÉÉÉÉÉ óó%cpp%cmm%cms"
upd:"DD%cMM%cRRRRR gg%cmm%css%cms"
"DD%cMM%cAAAAA hh%cmm%css"
"ДД%cММ%cРРРРР гг%cхх%cсс%cмс"

SetAttrTimeTitle3
"ГГГГГ%cММ%cДД чч%cмм%cсс%cмс"
"YYYYY%cMM%cDD hh%cmm%css%cms"
upd:"RRRRR%cMM%cDD hh%cmm%css%cms"
upd:"JJJJJ%cMM%cTT hh%cmm%css%cms"
upd:"ÉÉÉÉÉ%cHH%cNN óó%cpp%cmm%cms"
upd:"RRRRR%cMM%cDD gg%cmm%css%cms"
"AAAAA%cMM%cDD hh%cmm%css"
"ГГГГГ%cММ%cДД гг%cхх%cсс%cмс"

SetAttrBriefInfo
"&Краткое описание"
"Brief &info"
upd:"Brief &info"
upd:"Brief &info"
upd:"Brief &info"
upd:"Brief &info"
upd:"Brief &info"
upd:"Системні &властивості"

SetAttrSetting
l:
"Установка файловых атрибутов для"
"Setting file attributes for"
"Nastavení atributů souboru pro"
"Setze Dateiattribute für"
"Attribútumok beállítása"
"Ustawiam atrybuty"
"Poniendo atributos de archivo para"
"Встановлення файлових атрибутів для"

SetAttrCannotFor
"Ошибка установки атрибутов для"
"Cannot set attributes for"
"Nelze nastavit atributy pro"
"Konnte Dateiattribute nicht setzen für"
"Az attribútumok nem állíthatók be:"
"Nie mogę ustawić atrybutów dla"
"No se pueden poner atributos para"
"Помилка установки атрибутів для"

SetAttrTimeCannotFor
"Не удалось установить время файла для"
"Cannot set file time for"
"Nelze nastavit čas souboru pro"
"Konnte Dateidatum nicht setzen für"
"A dátum nem állítható be:"
"Nie mogę ustawić czasu pliku dla"
"No se puede poner hora de archivo para"
"Не вдалося встановити час для файлу"

SetAttrOwnerCannotFor
"Не удалось установить владельца для"
"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
"No se puede poner como dueño para"
"Не вдалося встановити власника для"

SetAttrGroupCannotFor
"Не удалось установить группу для"
"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
"Не вдалося встановити групу для"

SetAttrGroup
"Группа:"
"&Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
"Група:"

SetAttrAccessUser
"Права пользователя"
"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
"Права користувача"

SetAttrAccessGroup
"Права группы"
"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
"Права групи"

SetAttrAccessOther
"Права остальных"
"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
"Права інших"

SetAttrAccessUserRead
"&Чтение"
"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
"&Читання"

SetAttrAccessUserWrite
"&Запись"
"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
"&Запис"

SetAttrAccessUserExecute
"&Исполнение"
"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
"&Виконання"

SetAttrAccessGroupRead
"Чтение"
"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
"Читання"

SetAttrAccessGroupWrite
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

SetAttrAccessGroupExecute
"Исполнение"
"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
"Виконання"

SetAttrAccessOtherRead
"Чтение"
"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
"Читання"

SetAttrAccessOtherWrite
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

SetAttrAccessOtherExecute
"Исполнение"
"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
"Виконання"

SetAttrAccessTime
"Время последнего доступа"
"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
"Час останнього доступу"

SetAttrModificationTime
"Время последней &модификации"
"Last modification &time"
upd:"Last modification &time"
upd:"Last modification &time"
upd:"Last modification &time"
upd:"Last modification &time"
upd:"Last modification &time"
"Час останньої &модифікації"

SetAttrStatusChangeTime
"Время изменения статуса"
"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
"Час зміни статусу"

SetColorPanel
l:
"&Панель"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Панель"

SetColorDialog
"&Диалог"
"&Dialog"
"&Dialog"
"&Dialog"
"Pár&beszédablak"
"Okno &dialogowe"
"&Diálogo"
"&Діалог"

SetColorWarning
"Пр&едупреждение"
"&Warning message"
"&Varovná zpráva"
"&Warnmeldung"
"&Figyelmeztetés"
"&Ostrzeżenie"
"Me&nsaje de advertencia"
"Поп&ередження"

SetColorMenu
"&Меню"
"&Menu"
"&Menu"
"&Menü"
"&Menü"
"&Menu"
"&Menú"
"&Меню"

SetColorHMenu
"&Горизонтальное меню"
"Hori&zontal menu"
"Hori&zontální menu"
"Hori&zontales Menü"
"&Vízszintes menü"
"Pa&sek menu"
"Menú hori&zontal"
"&Горизонтальне меню"

SetColorKeyBar
"&Линейка клавиш"
"&Key bar"
"&Řádek kláves"
upd:"&Key bar"
"F&unkcióbill.sor"
"Pasek &klawiszy"
"Barra de me&nú"
"&Лінійка клавиш"

SetColorCommandLine
"&Командная строка"
"&Command line"
"Pří&kazový řádek"
"&Kommandozeile"
"P&arancssor"
"&Linia poleceń"
"Línea de &comando"
"&Командна строка"

SetColorClock
"&Часы"
"C&lock"
"&Hodiny"
"U&hr"
"Ó&ra"
"&Zegar"
"Re&loj"
"&Годинник"

SetColorViewer
"Про&смотрщик"
"&Viewer"
"P&rohlížeč"
"&Betrachter"
"&Nézőke"
"Pod&gląd"
"&Visor"
"Пере&глядач"

SetColorEditor
"&Редактор"
"&Editor"
"&Editor"
"&Editor"
"&Szerkesztő"
"&Edytor"
"&Editor"
"&Редактор"

SetColorHelp
"П&омощь"
"&Help"
"&Nápověda"
"&Hilfe"
"Sú&gó"
"P&omoc"
"&Ayuda"
"Д&опомога"

SetDefaultColors
"&Установить стандартные цвета"
"Set de&fault colors"
"N&astavit výchozí barvy"
"Setze Standard&farben"
"Alapért. s&zínek"
"&Ustaw kolory domyślne"
"Poner colores prede&terminados"
"&Встановити стандартні кольори"

SetBW
"Чёрно-бел&ый режим"
"&Black and white mode"
"Černo&bílý mód"
"Schwarz && &Weiß"
"Fekete-fe&hér mód"
"&Tryb czarno-biały"
"Modo &blanco y negro"
"Чорно-біл&ий режим"

SetColorPanelNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorPanelSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorPanelHighlightedInfo
"Выделенная информация"
"Highlighted info"
"Info zvýrazněné"
"Markierung"
"Kiemelt info"
"Podświetlone info"
"Info resaltados"
"Виділена інформація"

SetColorPanelDragging
"Перетаскиваемый текст"
"Dragging text"
"Tažený text"
"Drag && Drop Text"
"Vonszolt szöveg"
"Przeciągany tekst"
"Texto arrastrado"
"Перетягуваний текст"

SetColorPanelBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorPanelNormalCursor
"Обычный курсор"
"Normal cursor"
"Normální kurzor"
"Normale Auswahl"
"Normál kurzor"
"Normalny kursor"
"Cursor normal"
"Звичайний курсор"

SetColorPanelSelectedCursor
"Выделенный курсор"
"Selected cursor"
"Vybraný kurzor"
"Markierte Auswahl"
"Kijelölt kurzor"
"Wybrany kursor"
"Cursor seleccionado"
"Виділений курсор"

SetColorPanelNormalTitle
"Обычный заголовок"
"Normal title"
"Normální nadpis"
"Normaler Titel"
"Normál név"
"Normalny tytuł"
"Título normal"
"Звичайний заголовок"

SetColorPanelSelectedTitle
"Выделенный заголовок"
"Selected title"
"Vybraný nadpis"
"Markierter Titel"
"Kijelölt név"
"Wybrany tytuł"
"Título seleccionado"
"Виділелений заголовок"

SetColorPanelColumnTitle
"Заголовок колонки"
"Column title"
"Nadpis sloupce"
"Spaltentitel"
"Oszlopnév"
"Tytuł kolumny"
"Título de columna"
"Заголовок колонки"

SetColorPanelTotalInfo
"Количество файлов"
"Total info"
"Info celkové"
"Gesamtinfo"
"Összes info"
"Całkowite info"
"Info total"
"Кількість файлів"

SetColorPanelSelectedInfo
"Количество выбранных файлов"
"Selected info"
"Info výběr"
"Markierungsinfo"
"Kijelölt info"
"Wybrane info"
"Info seleccionados"
"Кількість вибраних файлів"

SetColorPanelScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

SetColorPanelScreensNumber
"Количество фоновых экранов"
"Number of background screens"
"Počet obrazovek na pozadí"
"Anzahl an Hintergrundseiten"
"Háttérképernyők száma"
"Ilość ekranów w tle "
"Número de pantallas de fondo"
"Кількість фонових екранів"

SetColorDialogNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Tekst zwykły"
"Texto normal"
"Звичайний текст"

SetColorDialogHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierter Text"
"Kiemelt szöveg"
"Tekst podświetlony"
"Texto resaltado"
"Виділений текст"

SetColorDialogDisabled
"Блокированный текст"
"Disabled text"
"Zakázaný text"
"Deaktivierter Text"
"Inaktív szöveg"
"Tekst nieaktywny"
"Deshabilitar texto"
"Блокований текст"

SetColorDialogBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorDialogBoxTitle
"Заголовок рамки"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок рамки"

SetColorDialogHighlightedBoxTitle
"Выделенный заголовок рамки"
"Highlighted title"
"Zvýrazněný nadpis"
"Markierter Titel"
"Kiemelt keretnév"
"Podświetlony tytuł"
"Título resaltado"
"Виділений заголовок рамки"

SetColorDialogTextInput
"Ввод текста"
"Text input"
"Textový vstup"
"Texteingabe"
"Beírt szöveg"
"Wpisywany tekst"
"Entrada de texto"
"Введення тексту"

SetColorDialogUnchangedTextInput
"Неизмененный текст"
"Unchanged text input"
"Nezměněný textový vstup"
"Unveränderte Texteingabe"
"Változatlan beírt szöveg"
"Niezmieniony wpisywany tekst "
"Entrada de texto sin cambiar"
"Незмінений текст"

SetColorDialogSelectedTextInput
"Ввод выделенного текста"
"Selected text input"
"Vybraný textový vstup"
"Markierte Texteingabe"
"Beírt szöveg kijelölve"
"Zaznaczony wpisywany tekst"
"Entrada de texto seleccionada"
"Введення виділеного тексту"

SetColorDialogEditDisabled
"Блокированное поле ввода"
"Disabled input line"
"Zakázaný vstupní řádek"
"Deaktivierte Eingabezeile"
"Inaktív beviteli sor"
"Nieaktywna linia wprowadzania danych"
"Deshabilitar línea de entrada"
"Блоковане поле вводу"

SetColorDialogButtons
"Кнопки"
"Buttons"
"Tlačítka"
"Schaltflächen"
"Gombok"
"Przyciski"
"Botones"
"Кнопки"

SetColorDialogSelectedButtons
"Выбранные кнопки"
"Selected buttons"
"Vybraná tlačítka"
"Aktive Schaltflächen"
"Kijelölt gombok"
"Wybrane przyciski"
"Botones seleccionados"
"Вибрані кнопки"

SetColorDialogHighlightedButtons
"Выделенные кнопки"
"Highlighted buttons"
"Zvýrazněná tlačítka"
"Markierte Schaltflächen"
"Kiemelt gombok"
"Podświetlone przyciski"
"Botones resaltados"
"Виділені кнопки"

SetColorDialogSelectedHighlightedButtons
"Выбранные выделенные кнопки"
"Selected highlighted buttons"
"Vybraná zvýrazněná tlačítka"
"Aktive markierte Schaltflächen"
"Kijelölt kiemelt gombok"
"Wybrane podświetlone przyciski "
"Botones resaltados seleccionados"
"Вибрані виділені кнопки"

SetColorDialogDefaultButton
"Кнопка по умолчанию"
"Default button"
upd:"Default button"
upd:"Default button"
upd:"Default button"
upd:"Default button"
"Botón por defecto"
"Кнопка за замовчуванням"

SetColorDialogSelectedDefaultButton
"Выбранная кнопка по умолчанию"
"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
"Botón por defecto seleccionado"
"Вибрана кнопка за замовчуванням"

SetColorDialogHighlightedDefaultButton
"Выделенная кнопка по умолчанию"
"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
"Botón por defecto resaltado"
"Виділена кнопка за замовчуванням"

SetColorDialogSelectedHighlightedDefaultButton
"Выбранная выделенная кнопка по умолчанию"
"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
"Botón por defecto resaltado seleccionado"
"Вибрана кнопка за замовчуванням"

SetColorDialogListBoxControl
"Список"
"List box"
"Seznam položek"
"Listenfelder"
"Listaablak"
"Lista"
"Cuadro de lista"
"Список"

SetColorDialogComboBoxControl
"Комбинированный список"
"Combobox"
"Výběr položek"
"Kombinatiosfelder"
"Lenyíló szövegablak"
"Pole combo"
"Cuadro combo"
"Комбінований список"

SetColorDialogListText
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Tekst zwykły"
"Texto normal"
"Звичайний текст"

SetColorDialogListSelectedText
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Tekst wybrany"
"Texto seleccionado"
"Вибраний текст"

SetColorDialogListHighLight
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Tekst podświetlony"
"Texto resaltado"
"Виділений текст"

SetColorDialogListSelectedHighLight
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Tekst wybrany i podświetlony"
"Texto resaltado seleccionado"
"Вибраний виділений текст"

SetColorDialogListDisabled
"Блокированный пункт"
"Disabled item"
"Naktivní položka"
"Deaktiviertes Element"
"Inaktív elem"
"Pole nieaktywne"
"Deshabilitar ítem"
"Блокований пункт"

SetColorDialogListBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorDialogListTitle
"Заголовок"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок"

SetColorDialogListGrayed
"Серый текст списка"
"Grayed list text"
upd:"Grayed list text"
upd:"Grayed list text"
"Szürke listaszöveg"
upd:"Grayed list text"
"Texto de listado en gris"
"Сірий текст списку"

SetColorDialogSelectedListGrayed
"Выбранный серый текст списка"
"Selected grayed list text"
upd:"Selected grayed list text"
upd:"Selected grayed list text"
"Kijelölt szürke listaszöveg"
upd:"Selected grayed list text"
"Texto de listado en gris seleccionado"
"Вибраний сірий текст списку"

SetColorDialogListScrollBar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

SetColorDialogListArrows
"Индикаторы длинных строк"
"Long string indicators"
"Značka dlouhého řetězce"
"Indikator für lange Zeichenketten"
"Hosszú sztring jelzők"
"Znacznik długiego napisu"
"Indicadores de cadena larga"
"Індикатори довгих рядків"

SetColorDialogListArrowsSelected
"Выбранные индикаторы длинных строк"
"Selected long string indicators"
"Vybraná značka dlouhého řetězce"
"Aktiver Indikator"
"Kijelölt hosszú sztring jelzők"
"Zaznaczone znacznik długiego napisu"
"Indicadores de cadena larga seleccionados"
"Вибрані індикатори довгих рядків"

SetColorDialogListArrowsDisabled
"Блокированные индикаторы длинных строк"
"Disabled long string indicators"
"Zakázaná značka dlouhého řetězce"
"Deaktivierter Indikator"
"Inaktív hosszú sztring jelzők"
"Nieaktywny znacznik długiego napisu"
"Deshabilitar indicadores de cadena largos"
"Блоковані індикатори довгих рядків"

SetColorMenuNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorMenuSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorMenuHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

SetColorMenuSelectedHighlighted
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Wybrany podświetlony tekst "
"Texto resaltado seleccionado"
"Вибраний виділений текст"

SetColorMenuDisabled
"Недоступный пункт"
"Disabled text"
"Neaktivní text"
"Disabled text"
"Inaktív szöveg"
"Tekst nieaktywny"
"Deshabilitar texto"
"Недоступний пункт"

SetColorMenuGrayed
"Серый текст"
"Grayed text"
upd:"Grayed text"
upd:"Grayed text"
"Szürke szöveg"
upd:"Grayed text"
"Texto en gris"
"Сірий текст"

SetColorMenuSelectedGrayed
"Выбранный серый текст"
"Selected grayed text"
upd:"Selected grayed text"
upd:"Selected grayed text"
"Kijelölt szürke szöveg"
upd:"Selected grayed text"
"Texto en gris seleccionado"
"Вибраний сірий текст"

SetColorMenuBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorMenuTitle
"Заголовок"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок"

SetColorMenuScrollBar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

SetColorMenuArrows
"Индикаторы длинных строк"
"Long string indicators"
"Značka dlouhého řetězce"
"Long string indicators"
"Hosszú sztring jelzők"
"Znacznik długiego napisu"
"Indicadores de cadena larga"
"Індикатори довгих рядків"

SetColorMenuArrowsSelected
"Выбранные индикаторы длинных строк"
"Selected long string indicators"
"Vybraná značka dlouhého řetězce"
"Selected long string indicators"
"Kijelölt hosszú sztring jelzők"
"Zaznaczone znacznik długiego napisu"
"Indicadores de cadena larga seleccionados"
"Вибрані індикатори довгих рядків"

SetColorMenuArrowsDisabled
"Блокированные индикаторы длинных строк"
"Disabled long string indicators"
"Zakázaná značka dlouhého řetězce"
"Disabled long string indicators"
"Inaktív hosszú sztring jelzők"
"Nieaktywny znacznik długiego napisu"
"Deshabilitar indicadores de cadena largos"
"Блоковані індикатори довгих рядків"

SetColorHMenuNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorHMenuSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorHMenuHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

SetColorHMenuSelectedHighlighted
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Wybrany podświetlony tekst "
"Texto resaltado seleccionado"
"Вибраний виділений текст"

SetColorKeyBarNumbers
l:
"Номера клавиш"
"Key numbers"
"Čísla kláves"
"Tastenziffern"
"Funkció száma"
"Numery klawiszy"
"Números teclas"
"Номери клавіш"

SetColorKeyBarNames
"Названия клавиш"
"Key names"
"Názvy kláves"
"Tastennamen"
"Funkció neve"
"Nazwy klawiszy"
"Nombres teclas"
"Назви клавіш"

SetColorKeyBarBackground
"Фон"
"Background"
"Pozadí"
"Hintergrund"
"Háttere"
"Tło"
"Fondo"
"Фон"

SetColorCommandLineNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorCommandLineSelected
"Выделенный текст"
"Selected text input"
"Vybraný textový vstup"
"Markierte Texteingabe"
"Beírt szöveg kijelölve"
"Zaznaczony wpisany tekst"
"Entrada de texto seleccionada"
"Виділелений текст"

SetColorCommandLinePrefix
"Текст префикса"
"Prefix text"
"Text předpony"
"Prefix Text"
"Előtag szövege"
"Tekst prefiksu"
"Texto prefijado"
"Текст префіксу"

SetColorCommandLineUserScreen
"Пользовательский экран"
"User screen"
"Obrazovka uživatele"
"Benutzerseite"
"Konzol háttere"
"Ekran użytkownika"
"Pantalla de usuario"
"Користувацький екран"

SetColorClockNormal
l:
"Обычный текст (панели)"
"Normal text (Panel)"
"Normální text (Panel)"
"Normaler Text (Panel)"
"Normál szöveg (panelek)"
"Normalny tekst (Panel)"
"Texto normal (Panel)"
"Звичайний текст (панелі)"

SetColorClockNormalEditor
"Обычный текст (редактор)"
"Normal text (Editor)"
"Normální text (Editor)"
"Normaler Text (Editor)"
"Normál szöveg (szerkesztő)"
"Normalny tekst (Edytor)"
"Texto normal (Editor)"
"Звичайний текст (редактор)"

SetColorClockNormalViewer
"Обычный текст (вьювер)"
"Normal text (Viewer)"
"Normální text (Prohlížeč)"
"Normaler Text (Betrachter)"
"Normál szöveg (nézőke)"
"Normalny tekst (Podgląd)"
"Texto normal (Visor)"
"Звичайний текст (в'ювер)"

SetColorViewerNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorViewerSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Zaznaczony tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorViewerStatus
"Статус"
"Status line"
"Stavový řádek"
"Statuszeile"
"Állapotsor"
"Linia statusu"
"Línea de estado"
"Статус"

SetColorViewerArrows
"Стрелки сдвига экрана"
"Screen scrolling arrows"
"Skrolovací šipky"
"Pfeile auf Scrollbalken"
"Képernyőgördítő nyilak"
"Strzałki przesuwające ekran"
"Flechas desplazamiento de pantalla"
"Стрілки зсуву екрана"

SetColorViewerScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barras desplazamiento"
"Полоса прокрутки"

SetColorEditorNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorEditorSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Zaznaczony tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorEditorStatus
"Статус"
"Status line"
"Stavový řádek"
"Statuszeile"
"Állapotsor"
"Linia statusu"
"Línea de estado"
"Статус"

SetColorEditorScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra de desplazamiento"
"Полоса прокрутки"

SetColorHelpNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorHelpHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

SetColorHelpReference
"Ссылка"
"Reference"
"Odkaz"
"Referenz"
"Hivatkozás"
"Odniesienie"
"Referencia"
"Посилання"

SetColorHelpSelectedReference
"Выбранная ссылка"
"Selected reference"
"Vybraný odkaz"
"Ausgewählte Referenz"
"Kijelölt hivatkozás"
"Wybrane odniesienie "
"Referencia seleccionada"
"Вибране посилання"

SetColorHelpBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorHelpBoxTitle
"Заголовок рамки"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок рамки"

SetColorHelpScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

SetColorGroupsTitle
l:
"Цветовые группы"
"Color groups"
"Skupiny barev"
"Farbgruppen"
"Színcsoportok"
"Grupy kolorów"
"Grupos de colores"
"Кольорові групи"

SetColorItemsTitle
"Элементы группы"
"Group items"
"Položky skupin"
"Gruppeneinträge"
"A színcsoport elemei"
"Elementy grupy"
"Grupos de ítems"
"Елементи групи"

SetColorTitle
l:
"Цвет"
"Color"
"Barva"
"Farbe"
"Színek"
"Kolor"
"Color"
"Колір"

SetColorForeground
"&Текст"
"&Foreground"
"&Popředí"
"&Vordergrund"
"&Előtér"
"&Pierwszy plan"
"&Caracteres"
"&Текст"

SetColorBackground
"&Фон"
"&Background"
"Po&zadí"
"&Hintergrund"
"&Háttér"
"&Tło"
"&Fondo     "
"&Фон"

SetColorForeTransparent
"&Прозрачный"
"&Transparent"
"Průhlednos&t"
"&Transparent"
"Átlá&tszó"
"P&rzezroczyste"
"&Transparente"
"&Прозорий"

SetColorBackTransparent
"П&розрачный"
"T&ransparent"
"Průhledno&st"
"T&ransparent"
"Átlát&szó"
"Pr&zezroczyste"
"T&ransparente"
"П&розорий"

SetColorSample
"Текст Текст Текст Текст Текст Текст"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Tekst Tekst Tekst Tekst Tekst Tekst"
"Texto Texto Texto Texto Texto"
"Текст Текст Текст Текст Текст Текст"

SetColorSet
"Установить"
"Set"
"Nastavit"
"Setzen"
"A&lkalmaz"
"Ustaw"
"Poner"
"Встановити"

SetColorCancel
"Отменить"
"Cancel"
"Storno"
"Abbruch"
"&Mégsem"
"Anuluj"
"Cancelar"
"Скасувати"

SetConfirmTitle
l:
"Подтверждения"
"Confirmations"
"Potvrzení"
"Bestätigungen"
"Megerősítések"
"Potwierdzenia"
"Confirmaciones"
"Підтвердження"

SetConfirmCopy
"Перезапись файлов при &копировании"
"&Copy"
"&Kopírování"
"&Kopieren"
"&Másolás"
"&Kopiowanie"
"&Copiar"
"Перезаписування файлів під час &копіювання"

SetConfirmMove
"Перезапись файлов при &переносе"
"&Move"
"&Přesouvání"
"&Verschieben"
"Moz&gatás"
"&Przenoszenie"
"&Mover"
"Перезаписування файлів під час &перенесення"

SetConfirmRO
"Перезапись и удаление R/O &файлов"
"&Overwrite and delete R/O files"
upd:"&Overwrite and delete R/O files"
upd:"&Overwrite and delete R/O files"
"&Csak olv. fájlok felülírása/törlése"
upd:"&Overwrite and delete R/O files"
"S&obrescribir y eliminar ficheros Sólo/Lectura"
"Перезапис та видалення R/O файлів"

SetConfirmDelete
"&Удаление"
"De&lete"
"&Mazání"
"&Löschen"
"&Törlés"
"&Usuwanie"
"&Borrar"
"&Видалення"

SetConfirmDeleteFolders
"У&даление непустых папок"
"Delete non-empty &folders"
"Mazat &neprázdné adresáře"
"Löschen von Ordnern mit &Inhalt"
"Nem &üres mappák törlése"
"Usuwanie &niepustych katalogów"
"Borrar &directorios no-vacíos"
"Ви&далення непорожніх тек"

SetConfirmEsc
"Прерыва&ние операций"
"&Interrupt operation"
"Pře&rušit operaci"
"&Unterbrechen von Vorgängen"
"Mű&velet megszakítása"
"&Przerwanie operacji"
"&Interrumpir operación"
"Перерива&ння операцій"

SetConfirmRemoveConnection
"&Отключение сетевого устройства"
"Disconnect &network drive"
"Odpojení &síťové jednotky"
"Trennen von &Netzwerklaufwerken"
"Háló&zati meghajtó leválasztása"
"Odłączenie dysku &sieciowego"
"Desconectar u&nidad de red"
"&Відключення мережного пристрою"

SetConfirmRemoveHotPlug
"Отключение HotPlug-у&стройства"
"Hot&Plug-device removal"
"Odpojení vyjímatelného zařízení"
"Sicheres Entfernen von Hardware"
"H&otPlug eszköz eltávolítása"
"Odłączanie urządzenia HotPlug"
"Remover dispositivo de conexión"
"Вимкнення HotPlug-п&ристроя"

SetConfirmAllowReedit
"Повто&рное открытие файла в редакторе"
"&Reload edited file"
"&Obnovit upravovaný soubor"
"Bea&rbeitete Datei neu laden"
"&Szerkesztett fájl újratöltése"
"&Załaduj edytowany plik"
"&Recargar archivo editado"
"Повто&рне відкриття файлу в редакторі"

SetConfirmHistoryClear
"Очистка списка &истории"
"Clear &history list"
"Vymazat seznam &historie"
"&Historielisten löschen"
"&Előzménylista törlése"
"Czyszczenie &historii"
"Limpiar listado de &historial"
"Очищення списку &історії"

SetConfirmExit
"&Выход"
"E&xit"
"U&končení"
"Be&enden"
"K&ilépés a FAR-ból"
"&Wyjście"
"&Salir"
"&Виход"

PluginsManagerSettingsTitle
l:
"Параметры менеджера внешних модулей"
"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
"Параметри менеджера зовнішніх модулів"

PluginsManagerScanSymlinks
"Ск&анировать символические ссылки"
"Scan s&ymbolic links"
"Prohledávat s&ymbolické linky"
"S&ymbolische Links scannen"
"Szimbolikus linkek &vizsgálata"
"Skanuj linki s&ymboliczne"
"Explorar enlaces simbólicos"
"Ск?анувати символічні посилання"

PluginsManagerPersonalPath
"Путь к персональным п&лагинам:"
"&Path for personal plugins:"
"&Cesta k vlastním pluginům:"
"&Pfad für eigene Plugins:"
"Saját plu&ginek útvonala:"
"Ś&cieżka do własnych pluginów:"
"Ruta para pl&ugins personales:
"Шлях до персональних п&лагінів:"

PluginsManagerOFP
"Обработка &файла (OpenFilePlugin)"
"&File processing (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
"&Fájl feldolgozása (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
"Proceso de archivo (OpenFilePlugin)"
"Обробка &файлу (OpenFilePlugin)"

PluginsManagerStdAssoc
"Пункт вызова стандартной &ассоциации"
"Show standard &association item"
upd:"Show standard &association item"
upd:"Show standard &association item"
"Szabvány társítás megjelenítése"
upd:"Show standard &association item"
"Mostrar asociaciones normales de ítems"
"Пункт виклику стандартної &асоціації"

PluginsManagerEvenOne
"Даже если найден всего &один плагин"
"Even if only &one plugin found"
upd:"Even if only &one plugin found"
upd:"Even if only &one plugin found"
"Akkor is, ha csak egy plugin van"
upd:"Even if only &one plugin found"
"Aún si solo se encontr un plugin"
"Навіть якщо знайдено всього &один плагін"

PluginsManagerSFL
"&Результаты поиска (SetFindList)"
"Search &results (SetFindList)"
upd:"Search &results (SetFindList)"
upd:"Search &results (SetFindList)"
"Keresés eredménye (SetFindList)"
upd:"Search &results (SetFindList)"
"Resultados de búsqueda (SetFindList)"
"&Результати пошуку (SetFindList)"

PluginsManagerPF
"Обработка &префикса"
"&Prefix processing"
upd:"&Prefix processing"
upd:"&Prefix processing"
"Előtag feldolgozása"
upd:"&Prefix processing"
"Proceso de prefijo"
"Обробка &префіксу"

PluginConfirmationTitle
"Выбор плагина"
"Plugin selection"
upd:"Plugin selection"
upd:"Plugin selection"
"Plugin választás"
upd:"Plugin selection"
"Selección de plugin"
"Вибір плагіна"

MenuPluginStdAssociation
"Стандартная ассоциация"
"Standard association"
upd:"Standard association"
upd:"Standard association"
"Szabvány társítás"
upd:"Standard association"
"Asociación normal"
"Стандартна асоціація"

FindFolderTitle
l:
"Поиск папки"
"Find folder"
"Najít adresář"
"Ordner finden"
"Mappakeresés"
"Znajdź folder"
"Encontrar directorio"
"Пошук теки"

KBFolderTreeF1
l:
l:// Find folder Tree KeyBar
"Помощь"
"Help"
"Nápověda"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

KBFolderTreeF2
"Обновить"
"Rescan"
"Obnovit"
"Aktual"
"FaFris"
"Czytaj ponownie"
"ReExpl"
"Оновити"

KBFolderTreeF5
"Размер"
"Zoom"
"Zoom"
"Vergr."
"Nagyít"
"Powiększ"
"Zoom"
"Розмір"

KBFolderTreeF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

KBFolderTreeAltF9
"Видео"
"Video"
"Video"
"Vollb"
"Video"
"Video"
"Video"
"Відео"

TreeTitle
"Дерево"
"Tree"
"Stromové zobrazení"
"Baum"
"Fa"
"Drzewo"
"Arbol"
"Дерево"

CannotSaveTree
"Ошибка записи дерева папок в файл"
"Cannot save folders tree to file"
"Adresářový strom nelze uložit do souboru"
"Konnte Ordnerliste nicht in Datei speichern."
"A mappák fastruktúrája nem menthető fájlba"
"Nie mogę zapisać drzewa katalogów do pliku"
"No se puede guardar árbol de directorios al archivo"
"Помилка запису дерева папок у файл"

ReadingTree
"Чтение дерева папок"
"Reading the folders tree"
"Načítám adresářový strom"
"Lese Ordnerliste"
"Mappaszerkezet újraolvasása..."
"Odczytuję drzewo katalogów"
"Leyendo árbol de directorios"
"Читання дерева папок"

UserMenuTitle
l:
"Пользовательское меню"
"User menu"
"Menu uživatele"
"Benutzermenü"
"Felhasználói menü szerkesztése"
"Menu użytkownika"
"Menú de usuario"
"Користувачське меню"

ChooseMenuType
"Выберите тип пользовательского меню для редактирования"
"Choose user menu type to edit"
"Zvol typ menu uživatele pro úpravu"
"Wählen Sie den Typ des zu editierenden Benutzermenüs"
"Felhasználói menü típusa:"
"Wybierz typ menu do edycji"
"Elija tipo de menú usuario a editar"
"Виберіть тип меню користувача для редагування"

ChooseMenuMain
"&Главное"
"&Main"
"&Hlavní"
"&Hauptmenü"
"&Főmenü"
"Główne"
"&Principal"
"&Головне"

ChooseMenuLocal
"&Местное"
"&Local"
"&Lokální"
"&Lokales Menü"
"&Helyi menü"
"Lokalne"
"&Local"
"&Місцеве"

MainMenuTitle
"Главное меню"
"Main menu"
"Hlavní menu"
"Hauptmenü"
"Főmenü"
"Menu główne"
"Menú principal"
"Головне меню"

MainMenuFAR
"Папка FAR"
"FAR folder"
"Složka FARu"
"FAR Ordner"
"FAR mappa"
"Folder FAR-a"
"Directorio FAR"
"Тека FAR"

MainMenuREG
l:
l:// <...menu (Registry)>
"Реестр"
"Registry"
"Registry"
"Reg."
"Registry"
"Rejestr"
"Registro"
"Реєстр"

LocalMenuTitle
"Местное меню"
"Local menu"
"Lokalní menu"
"Lokales Menü"
"Helyi menü"
"Menu lokalne"
"Menú local"
"Місцеве меню"

MainMenuBottomTitle
"Редактирование: Del,Ins,F4,Ctrl-F4"
"Edit: Del,Ins,F4,Ctrl-F4"
"Edit: Del,Ins,F4,Ctrl-F4"
"Bearb.: Entf,Einf,F4,Ctrl-F4"
"Szerk.: Del,Ins,F4,Ctrl-F4"
"Edycja: Del,Ins,F4,Ctrl-F4"
"Editar: Del,Ins,F4"
"Редагування: Del,Ins,F4,Ctrl-F4"

AskDeleteMenuItem
"Вы хотите удалить пункт меню"
"Do you wish to delete the menu item"
"Přejete si smazat položku v menu"
"Do you wish to delete the menu item"
"Biztosan törli a menüelemet?"
"Czy usunąć pozycję menu"
"Desea borrar el ítem del menú"
"Ви хочете видалити пункт меню"

AskDeleteSubMenuItem
"Вы хотите удалить вложенное меню"
"Do you wish to delete the submenu"
"Přejete si smazat podmenu"
"Do you wish to delete the submenu"
"Biztosan törli az almenüt?"
"Czy usunąć podmenu"
"Desea borrar el submenú"
"Ви хочете видалити вкладене меню"

UserMenuInvalidInputLabel
"Неправильный формат метки меню"
"Invalid format for UserMenu Label"
"Neplatný formát pro název Uživatelského menu"
"Invalid format for UserMenu Label"
"A felhasználói menü névformátuma érvénytelen"
"Błędny format etykiety menu użytkownika"
"Formato inválido para etiqueta de menú usuario"
"Неправильний формат мітки меню"

UserMenuInvalidInputHotKey
"Неправильный формат горячей клавиши"
"Invalid format for Hot Key"
"Neplatný formát pro klávesovou zkratku"
"Invalid format for Hot Key"
"A gyorsbillentyű formátuma érvénytelen"
"Błędny format klawisza skrótu"
"Formato inválido para tecla rápida"
"Неправильний формат гарячої клавіші"

EditMenuTitle
l:
"Редактирование пользовательского меню"
"Edit user menu"
"Editace uživatelského menu"
"Menübefehl bearbeiten"
"Parancs szerkesztése"
"Edytuj menu użytkownika"
"Editar menú de usuario"
"Редагування меню користувача"

EditMenuHotKey
"&Горячая клавиша:"
"&Hot key:"
"K&lávesová zkratka:"
"&Kurztaste:"
"&Gyorsbillentyű:"
"&Klawisz skrótu:"
"&Tecla rápida:"
"&Гаряча клавіша:"

EditMenuLabel
"&Метка:"
"&Label:"
"&Popisek:"
"&Bezeichnung:"
"&Név:"
"&Etykieta:"
"&Etiqueta:"
"&Мітка:"

EditMenuCommands
"&Команды:"
"&Commands:"
"Pří&kazy:"
"&Befehle:"
"&Parancsok:"
"&Polecenia:"
"&Comandos:"
"&Команди:"

AskInsertMenuOrCommand
l:
"Вы хотите вставить новую команду или новое меню?"
"Do you wish to insert a new command or a new menu?"
"Přejete si vložit nový příkaz nebo nové menu?"
"Wollen Sie einen neuen Menübefehl oder ein neues Menu erstellen?"
"Új parancs vagy új menü?"
"Czy chcesz wstawić nowe polecenie lub nowe menu?"
"Desea insertar un nuevo comando o un nuevo menú?"
"Ви хочете вставити нову команду або нове меню?"

MenuInsertCommand
"Вставить команду"
"Insert command"
"Vložit příkaz"
"Neuer Befehl"
"Parancs"
"Wstaw polecenie"
"Insertar comando"
"Вставити команду"

MenuInsertMenu
"Вставить меню"
"Insert menu"
"Vložit menu"
"Neues Menü"
"Menü"
"Wstaw menu"
"Insertar menú"
"Вставити меню"

EditSubmenuTitle
l:
"Редактирование метки вложенного меню"
"Edit submenu label"
"Úprava popisku podmenu"
"Untermenü bearbeiten"
"Almenü szerkesztése"
"Edytuj etykietę podmenu"
"Editar etiqueta de submenú"
"Редагування позначки вкладеного меню"

ViewerTitle
l:
"Просмотр"
"Viewer"
"Prohlížeč"
"Betrachter"
"Nézőke"
"Podgląd"
"Visor"
"Перегляд"

ViewerCannotOpenFile
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie mogę otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

ViewerStatusCol
"Кол"
"Col"
"Sloupec"
"Spalte"
"Oszlop"
"Kolumna"
"Col"
"Кол"

ViewSearchTitle
l:
"Поиск"
"Search"
"Hledat"
"Durchsuchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

ViewSearchFor
"&Искать"
"&Search for"
"H&ledat"
"&Suchen nach"
"&Keresés:"
"&Znajdź"
"&Buscar por"
"&Шукати"

ViewSearchForText
"Искать &текст"
"Search for &text"
"Hledat &text"
"Suchen nach &Text"
"&Szöveg keresése"
"Szukaj &tekstu"
"Buscar cadena de &texto"
"Шукати &текст"

ViewSearchForHex
"Искать 16-ричный &код"
"Search for &hex"
"Hledat he&x"
"Suchen nach &Hex (xx xx ...)"
"&Hexa keresése"
"Szukaj &wartości szesnastkowych"
"Buscar cadena &hexadecimal"
"Шукати 16-річний &код"

ViewSearchCase
"&Учитывать регистр"
"&Case sensitive"
"&Rozlišovat velikost písmen"
"Gr&oß-/Kleinschreibung"
"&Nagy/kisbetű érzékeny"
"&Uwzględnij wielkość liter"
"Sensible min/ma&yúsculas"
"&Враховувати регістр"

ViewSearchWholeWords
"Только &целые слова"
"&Whole words"
"Celá &slova"
"Ganze &Wörter"
"Csak e&gész szavak"
"Tylko całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

ViewSearchReverse
"Обратн&ый поиск"
"Re&verse search"
"&Zpětné hledání"
"Richtung um&kehren"
"&Visszafelé keres"
"Szukaj w &odwrotnym kierunku"
"Buscar al in&verso"
"Зворотн&ий пошук"

ViewSearchRegexp
"&Регулярные выражения"
"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
"Expresiones &regulares"
"&Регулярні вирази"

ViewSearchSearch
"Искать"
"Search"
"Hledat"
"Suchen"
"Keres"
"&Szukaj"
"Buscar"
"Шукать"

ViewSearchCancel
"Отменить"
"Cancel"
"Storno"
"Abbrechen"
"Mégsem"
"&Anuluj"
"Cancelar"
"Скасувати"

ViewSearchingFor
l:
"Поиск"
"Searching for"
"Vyhledávám"
"Suche nach"
"Keresés:"
"Szukam"
"Buscando por"
"Пошук"

ViewSearchingHex
"Поиск байтов"
"Searching for bytes"
"Vyhledávám sekvenci bytů"
"Suche nach Bytes"
"Bájtok keresése:"
"Szukam bajtów"
"Buscando por bytes"
"Пошук байтів"

ViewSearchCannotFind
"Строка не найдена"
"Could not find the string"
"Nelze najít řetězec"
"Konnte Zeichenkette nicht finden"
"Nem találtam a szöveget:"
"Nie mogę odnaleźć ciągu znaków"
"No se puede encontrar la cadena"
"Строка не знайдена"

ViewSearchCannotFindHex
"Байты не найдены"
"Could not find the bytes"
"Nelze najít sekvenci bytů"
"Konnte Bytefolge nicht finden"
"Nem találtam a bájtokat:"
"Nie mogę odnaleźć bajtów"
"No se puede encontrar los bytes"
"Байти не знайдені"

ViewSearchFromBegin
"Продолжить поиск с начала документа?"
"Continue the search from the beginning of the document?"
"Pokračovat s hledáním od začátku dokumentu?"
"Mit Suche am Anfang des Dokuments fortfahren?"
"Folytassam a keresést a dokumentum elejétől?"
"Kontynuować wyszukiwanie od początku dokumentu?"
"Continuar búsqueda desde el comienzo del documento"
"Продовжити пошук з початку документа?"

ViewSearchFromEnd
"Продолжить поиск с конца документа?"
"Continue the search from the end of the document?"
"Pokračovat s hledáním od konce dokumentu?"
"Mit Suche am Ende des Dokuments fortfahren?"
"Folytassam a keresést a dokumentum végétől?"
"Kontynuować wyszukiwanie od końca dokumentu?"
"Continuar búsqueda desde el final del documento"
"Продовжити пошук з кінця документа?"

DescribeFiles
l:
"Описание файла"
"Describe file"
"Popiskový soubor"
"Beschreibung ändern"
"Fájlmegjegyzés"
"Opisz plik"
"Describir archivos"
"Опис файлу"

EnterDescription
"Введите описание для"
"Enter description for"
"Zadejte popisek"
"Beschreibung für"
upd:"Írja be megjegyzését:"
"Wprowadź opis"
"Entrar descripción de %ls"
"Введіть опис для"

ReadingDiz
l:
"Чтение описаний файлов"
"Reading file descriptions"
"Načítám popisky souboru"
"Lese Dateibeschreibungen"
"Fájlmegjegyzések olvasása"
"Odczytuję opisy plików"
"Leyendo descripción de archivos"
"Читання описів файлів"

CannotUpdateDiz
"Не удалось обновить описания файлов"
"Cannot update file descriptions"
"Nelze aktualizovat popisky souboru"
"Dateibeschreibungen konnten nicht aktualisiert werden."
"A fájlmegjegyzések nem frissíthetők"
"Nie moge aktualizować opisów plików"
"No se puede actualizar descripción de archivos"
"Не вдалося оновити опис файлів"

CannotUpdateRODiz
"Файл описаний защищён от записи"
"The description file is read only"
"Popiskový soubor má atribut Jen pro čtení"
"Die Beschreibungsdatei ist schreibgeschützt."
"A megjegyzésfájl csak olvasható"
"Opis jest plikiem tylko do odczytu"
"El archivo descripción es de sólo lectura"
"Файл опису захищений від запису"

CfgDizTitle
l:
"Описания файлов"
"File descriptions"
"Popisky souboru"
"Dateibeschreibungen"
"Fájl megjegyzésfájlok"
"Opisy plików"
"Descripción de archivos"
"Опис файлів"

CfgDizListNames
"Имена &списков описаний, разделённые запятыми:"
"Description &list names delimited with commas:"
"Seznam pop&isových souborů oddělených čárkami:"
"Beschreibungs&dateien, getrennt durch Komma:"
"Megjegyzés&fájlok nevei, vesszővel elválasztva:"
"Nazwy &plików z opisami oddzielone przecinkami:"
"Nombres de &listas de descripción delimitado con comas:"
"Імена &списків описів, розділені комами:"

CfgDizSetHidden
"Устанавливать &атрибут ""Скрытый"" на новые списки описаний"
"Set ""&Hidden"" attribute to new description lists"
"Novým souborům s popisy nastavit atribut ""&Skrytý"""
"Setze das '&Versteckt'-Attribut für neu angelegte Dateien"
"Az új megjegyzésfájl ""&rejtett"" attribútumú legyen"
"Ustaw atrybut ""&Ukryty"" dla nowych plików z opisami"
"Poner atributo ""&Oculto"" a las nuevas listas de descripción"
"Встановлювати &атрибут ""Прихований"" на нові списки описів"

CfgDizROUpdate
"Обновлять файл описаний с атрибутом ""Толь&ко для чтения"""
"Update &read only description file"
"Aktualizovat popisové soubory s atributem Jen pro čtení"
"Schreibgeschützte Dateien aktualisie&ren"
"&Csak olvasható megjegyzésfájlok frissítése"
"Aktualizuj plik opisu tylko do odczytu"
"Actualizar archivo descripción de sólo lectura"
"Оновлювати файл опису з атрибутом ""Тіль&ки для читання"""

CfgDizStartPos
"&Позиция новых описаний в строке"
"&Position of new descriptions in the string"
"&Pozice nových popisů v řetězci"
"&Position neuer Beschreibungen in der Zeichenkette"
"Új megjegyzéseknél a szöveg &kezdete"
"Pozy&cja nowych opisów w linii"
"&Posición de nueva descripciones en la cadena"
"&Позиція нових описів у рядку"

CfgDizNotUpdate
"&Не обновлять описания"
"Do &not update descriptions"
"&Neaktualizovat popisy"
"Beschreibungen &nie aktualisieren"
"N&e frissítse a megjegyzéseket"
"&Nie aktualizuj opisów"
"&No actualizar descripciones"
"&Не оновлювати описи"

CfgDizUpdateIfDisplayed
"&Обновлять, если они выводятся на экран"
"Update if &displayed"
"Aktualizovat, jestliže je &zobrazen"
"Aktualisieren &wenn angezeigt"
"Frissítsen, ha meg&jelenik"
"Aktualizuj jeśli &widoczne"
"Actualizar si es visualiza&do"
"&Оновлювати, якщо вони відображаються на екрані"

CfgDizAlwaysUpdate
"&Всегда обновлять"
"&Always update"
"&Vždy aktualizovat"
"Im&mer aktualisieren"
"&Mindig frissítsen"
"&Zawsze aktualizuj"
"&Actualizar siempre"
"&Завжди оновлювати"

CfgDizAnsiByDefault
"&Использовать кодовую страницу ANSI по умолчанию"
"Use ANS&I code page by default"
upd:"Automaticky otevírat soubory ve &WIN kódování"
upd:"Dateien standardmäßig mit Windows-Kod&ierung öffnen"
"Fájlok eredeti megnyitása ANS&I kódlappal"
"&Otwieraj pliki w kodowaniu Windows"
"Usar código ANS&I por defecto"
"&Використовувати кодову сторінку ANSI за замовчуванням"

CfgDizSaveInUTF
"Сохранять в UTF8"
"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
"Guardar en UTF8"
"Зберігати в UTF8"

ReadingTitleFiles
l:
"Обновление панелей"
"Update of panels"
"Aktualizace panelů"
"Aktualisiere Panels"
"Panelek frissítése"
"Aktualizacja panelu"
"Actualizar paneles"
"Оновлення панелей"

ReadingFiles
"Чтение: %d файлов"
"Reading: %d files"
"Načítám: %d souborů"
"Lese: %d Dateien"
" %d fájl olvasása"
"Czytam: %d plików"
"Leyendo: %d archivos"
"Читання: %d файлів"

EditPanelModes
l:
"Режимы панели"
"Edit panel modes"
"Editovat módy panelu"
"Anzeigemodi von Panels bearbeiten"
"Panel módok szerkesztése"
"Edytuj tryby wyświetlania paneli"
"Editar modo de paneles"
"Режими панелі"

EditPanelModesBrief
l:
"&Краткий режим"
"&Brief mode"
"&Stručný mód"
"&Kurz"
"&Rövid mód"
"&Skrótowy"
"&Breve     "
"&Короткий режим"

EditPanelModesMedium
"&Средний режим"
"&Medium mode"
"S&třední mód"
"&Mittel"
"&Közepes mód"
"Ś&redni"
"&Medio      "
"&Середний режим"

EditPanelModesFull
"&Полный режим"
"&Full mode"
"&Plný mód"
"&Voll"
"&Teljes mód"
"&Pełny"
"&Completo "
"&Повний режим"

EditPanelModesWide
"&Широкий режим"
"&Wide mode"
"Š&iroký mód"
"B&reitformat"
"&Széles mód"
"S&zeroki"
"&Amplio   "
"&Широкий режим"

EditPanelModesDetailed
"&Детальный режим"
"Detai&led mode"
"Detai&lní mód"
"Detai&lliert"
"Rés&zletes mód"
"Ze sz&czegółami"
"Detal&lado    "
"&Детальний режим"

EditPanelModesDiz
"&Описания"
"&Descriptions mode"
"P&opiskový mód"
"&Beschreibungen"
"&Fájlmegjegyzés mód"
"&Opisy"
"&Descripción      "
"&Описи"

EditPanelModesLongDiz
"Д&линные описания"
"Lon&g descriptions mode"
"&Mód dlouhých popisků"
"Lan&ge Beschreibungen"
"&Hosszú megjegyzés mód"
"&Długie opisy"
"Descripción lar&ga"
"Д&овгі описи"

EditPanelModesOwners
"Вл&адельцы файлов"
"File own&ers mode"
"Mód vlastníka so&uborů"
"B&esitzer"
"T&ulajdonos mód"
"&Właściciele"
"Du&eños de archivos"
"Вл&асники файлів"

EditPanelModesLinks
"Свя&зи файлов"
"Lin&ks mode"
"Lin&kový mód"
"Dateilin&ks"
"Li&nkek mód"
"Dowiąza&nia"
"En&laces    "
"Зв'я&зки файлів"

EditPanelModesAlternative
"Аль&тернативный полный режим"
"&Alternative full mode"
"&Alternativní plný mód"
"&Alternative Vollansicht"
"&Alternatív teljes mód"
"&Alternatywny"
"Alternativo com&pleto "
"Аль&тернативний повний режим"

EditPanelModeTypes
l:
"&Типы колонок"
"Column &types"
"&Typ sloupců"
"Spalten&typen"
"Oszlop&típusok"
"&Typy kolumn"
"&Tipos de columna"
"&Типи колонок"

EditPanelModeWidths
"&Ширина колонок"
"Column &widths"
"Šíř&ka sloupců"
"Spalten&breiten"
"Oszlop&szélességek"
"&Szerokości kolumn"
"Anc&ho de columna"
"&Ширина колонок"

EditPanelModeStatusTypes
"Типы колонок строки ст&атуса"
"St&atus line column types"
"T&yp sloupců stavového řádku"
"St&atuszeile Spaltentypen"
"Állapotsor oszloptíp&usok"
"Typy kolumn &linii statusu"
"Tipos de columnas líne&a de estado"
"Типи колонок рядка ст&атусу"

EditPanelModeStatusWidths
"Ширина колонок строки стат&уса"
"Status l&ine column widths"
"Šířka slo&upců stavového řádku"
"Statusze&ile Spaltenbreiten"
"Állapotsor &oszlopszélességek"
"Szerokości kolumn l&inii statusu"
"Ancho de columnas lí&nea de estado"
"Ширина колонок рядка стат&усу"

EditPanelModeFullscreen
"&Полноэкранный режим"
"&Fullscreen view"
"&Celoobrazovkový režim"
"&Vollbild"
"Tel&jes képernyős nézet"
"Widok &pełnoekranowy"
"&Vista pantalla completa"
"&Повноекранний режим"

EditPanelModeAlignExtensions
"&Выравнивать расширения файлов"
"Align file &extensions"
"Zarovnat příp&ony souborů"
"Datei&erweiterungen ausrichten"
"Fájlkiterjesztések &igazítása"
"W&yrównaj rozszerzenia plików"
"Alinear &extensiones de archivos"
"&Вирівнювати розширення файлів"

EditPanelModeAlignFolderExtensions
"Выравнивать расширения пап&ок"
"Align folder e&xtensions"
"Zarovnat přípony adre&sářů"
"Ordnerer&weiterungen ausrichten"
"Mappakiterjesztések i&gazítása"
"Wyrównaj rozszerzenia &folderów"
"Alinear e&xtensiones de directorios"
"Вирівнювати розширення те&к"

EditPanelModeFoldersUpperCase
"Показывать папки &заглавными буквами"
"Show folders in &uppercase"
"Zobrazit adresáře &velkými písmeny"
"Ordner in Großb&uchstaben zeigen"
"Mappák NAG&YBETŰVEL mutatva"
"Nazwy katalogów &WIELKIMI LITERAMI"
"Directorios en mayú&sculas"
"Показувати теки &великими літерами"

EditPanelModeFilesLowerCase
"Показывать файлы ст&рочными буквами"
"Show files in &lowercase"
"Zobrazit soubory ma&lými písmeny"
"Dateien in K&leinbuchstaben zeigen"
"Fájlok kis&betűvel mutatva"
"&Nazwy plików małymi literami"
"archivos en minúscu&las"
"Показувати файли ма&лими літерами"

EditPanelModeUpperToLowerCase
"Показывать имена файлов из заглавных букв &строчными буквами"
"Show uppercase file names in lower&case"
"Zobrazit velké znaky ve jménech souborů jako &malá písmena"
"G&roßgeschriebene Dateinamen in Kleinbuchstaben zeigen"
"NAGYBETŰS fájl&nevek kisbetűvel"
"Wyświetl NAZWY_PLIKÓW &jako nazwy_plików"
"archivos en mayúsculas mostrarlos con minús&culas"
"Показувати імена файлів із великих літер &маленькими літерами"

EditPanelReadHelp
" Нажмите F1, чтобы получить информацию по настройке "
" Read online help for instructions "
" Pro instrukce si přečtěte online nápovědu "
" Siehe Hilfe für Anweisungen "
" Tanácsokat a súgóban talál (F1) "
" Instrukcje zawarte są w pomocy podręcznej "
" Para instrucciones leer ayuda en línea "
" Натисніть F1, щоб отримати інформацію про налаштування "

SetFolderInfoTitle
l:
"Файлы информации о папках"
"Folder description files"
"Soubory s popiskem adresáře"
"Ordnerbeschreibungen"
"Mappa megjegyzésfájlok"
"Pliki opisu katalogu"
"Descripciones de directorio"
"Файли інформації про теки"

SetFolderInfoNames
"Введите имена файлов, разделённые запятыми (допускаются маски)"
"Enter file names delimited with commas (wildcards are allowed)"
"Zadejte jména souborů oddělených čárkami (značky jsou povoleny)"
"Dateiliste, getrennt mit Komma (Jokerzeichen möglich):"
"Fájlnevek, vesszővel elválasztva (joker is használható)"
"Nazwy plików oddzielone przecinkami (znaki ? i * dopuszczalne)"
"Ingrese nombre de archivo delimitado con comas (comodines permitidos)"
"Введіть імена файлів, розділені комами (допускаються маски)"

ScreensTitle
l:
"Экраны"
"Screens"
"Obrazovky"
"Seiten"
"Képernyők"
"Ekrany"
"Pant.  "
"Екрани"

ScreensPanels
"Панели"
"Panels"
"Panely"
"Panels"
"Panelek"
"Panele"
"Paneles"
"Панелі"

ScreensView
"Просмотр"
"View"
"Zobrazit"
"Betr."
"Nézőke"
"Podgląd"
"Ver"
"Перегляд"

ScreensEdit
"Редактор"
"Edit"
"Editovat"
"Bearb"
"Szerkesztő"
"Edycja"
"Editar"
"Редактор"

AskApplyCommandTitle
l:
"Применить команду"
"Apply command"
"Aplikovat příkaz"
"Befehl anwenden"
"Parancs végrehajtása"
"Zastosuj polecenie"
"Aplicar comando"
"Примінити команду"

AskApplyCommand
"Введите команду для обработки выбранных файлов"
"Enter command to process selected files"
"Zadejte příkaz pro zpracování vybraných souborů"
"Befehlszeile auf ausgewählte Dateien anwenden:"
"Írja be a kijelölt fájlok parancsát:"
"Wprowadź polecenie do przetworzenia wybranych plików"
"Ingrese comando para procesar archivos seleccionados"
"Введіть команду для обробки вибраних файлів"

PluginConfigTitle
l:
"Конфигурация модулей"
"Plugins configuration"
"Nastavení Pluginů"
"Konfiguration von Plugins"
"Plugin beállítások"
"Konfiguracja pluginów"
"Configuración de plugins"
"Конфігурація модулів"

PluginCommandsMenuTitle
"Команды внешних модулей"
"Plugin commands"
"Příkazy pluginů"
"Pluginbefehle"
"Plugin parancsok"
"Dostępne pluginy"
"Comandos de plugins"
"Команди зовнішніх модулів"

PreparingList
l:
"Создание списка файлов"
"Preparing files list"
"Připravuji seznam souborů"
"Dateiliste wird vorbereitet"
"Fájllista elkészítése"
"Przygotowuję listę plików"
"Preparando lista de archivos"
"Створення списку файлів"

LangTitle
l:
"Основной язык"
"Main language"
"Hlavní jazyk"
"Hauptsprache"
"A program nyelve"
"Język programu"
"Idioma principal"
"Основна мова"

HelpLangTitle
"Язык помощи"
"Help language"
"Jazyk nápovědy"
"Sprache der Hilfedatei"
"A súgó nyelve"
"Język pomocy"
"Idioma de ayuda"
"Мова допомоги"

DefineMacroTitle
l:
"Задание макрокоманды"
"Define macro"
"Definovat makro"
"Definiere Makro"
"Makró gyorsbillentyű"
"Zdefiniuj makro"
"Definir macro"
"Завдання макрокоманди"

DefineMacro
"Нажмите желаемую клавишу"
"Press the desired key"
"Stiskněte požadovanou klávesu"
"Tastenkombination:"
"Nyomja le a billentyűt"
"Naciśnij żądany klawisz"
"Pulse la tecla deseada"
"Натисніть бажану клавішу"

MacroReDefinedKey
"Макроклавиша '%ls' уже определена."
"Macro key '%ls' already defined."
"Klávesa makra '%ls' již je definována."
"Makro '%ls' bereits definiert."
""%ls" makróbillentyű foglalt"
"Skrót '%ls' jest już zdefiniowany."
"Macro '%ls' ya está definido. Secuencia:"
"Макроклавіша '%ls' вже визначена."

MacroDeleteKey
"Макроклавиша '%ls' будет удалена."
"Macro key '%ls' will be removed."
"Klávesa makra '%ls' bude odstraněna."
"Makro '%ls' wird entfernt und ersetzt:"
""%ls" makróbillentyű törlődik"
"Skrót '%ls' zostanie usunięty."
"Macro '%ls' será removido. Secuencia:"
"Макроклавіша '%ls' буде видалена."

MacroCommonReDefinedKey
"Общая макроклавиша '%ls' уже определена."
"Common macro key '%ls' already defined."
"Klávesa pro běžné makro '%ls' již je definována."
"Gemeinsames Makro '%ls' bereits definiert."
""%ls" közös makróbill. foglalt"
"Skrót '%ls' jest już zdefiniowany."
"Tecla de macro '%ls' ya ha sido definida."
"Спільна макроклавіша '%ls' вже визначена."

MacroCommonDeleteKey
"Общая макроклавиша '%ls' будет удалена."
"Common macro key '%ls' will be removed."
"Klávesa pro běžné makro '%ls' bude odstraněna."
"Gemeinsames Makro '%ls' wird entfernt und ersetzt:"
""%ls" közös makróbill. törlődik"
"Skrót '%ls' zostanie usunięty."
"Tecla de macro '%ls' será removida."
"Спільна макроклавіша '%ls' буде видалена."

MacroSequence
"Последовательность:"
"Sequence:"
"Posloupnost:"
"Sequenz:"
"Szekvencia:"
"Sekwencja:"
"Secuencia:"
"Послідовність:"

MacroDescription
"Описание:"
"Description:"
"Popis:"
"Beschreibung:"
"Megjegyzés:"
"Opis:"
"Descripción:"
"Опис:"

MacroReDefinedKey2
"Переопределить?"
"Redefine?"
"Předefinovat?"
"Neu definieren?"
"Újradefiniálja?"
"Zdefiniować powtórnie?"
"Redefinir?"
"Перевизначити?"

MacroDeleteKey2
"Удалить?"
"Delete?"
"Odstranit?"
"Löschen?"
"Törli?"
"Usunąć?"
"Borrar?"
"Видалити?"

MacroEditKey
"Изменить"
"Chan&ge"
"Změnit"
"Verändern"
upd:"Change"
"Zmienić?"
"Cambiar?"
"Змінити"

MacroSettingsTitle
l:
"Параметры макрокоманды для '%ls'"
"Macro settings for '%ls'"
"Nastavení makra pro '%ls'"
"Einstellungen für Makro '%ls'"
""%ls" makró beállításai"
"Ustawienia makra dla '%ls'"
"Configurar macro para '%ls'"
"Параметри макрокоманди для '%ls'"

MacroSettingsEnableOutput
"Разрешить во время &выполнения вывод на экран"
"Allo&w screen output while executing macro"
"Povolit &výstup na obrazovku dokud se provádí makro"
"Bildschirmausgabe &während Makro abläuft"
"Képernyő&kimenet a makró futása közben"
"&Wyłącz zapis na ekran podczas wykonywania makra"
"Permitir salida pantalla mientras se ejecut&an los macros"
"Дозволити під час виконання &виведення на екран"

MacroSettingsRunAfterStart
"В&ыполнять после запуска FAR"
"Execute after FAR &start"
"&Spustit po spuštění FARu"
"Ausführen beim &Starten von FAR"
"Végrehajtás a FAR &indítása után"
"Wykonaj po &starcie FAR-a"
"Ejecutar luego de &iniciar FAR"
"В&иконати після запуску FAR"

MacroSettingsActivePanel
"&Активная панель"
"&Active panel"
"&Aktivní panel"
"&Aktives Panel"
"&Aktív panel"
"Panel &aktywny"
"Panel &activo"
"&Активна панель"

MacroSettingsPassivePanel
"&Пассивная панель"
"&Passive panel"
"Pa&sivní panel"
"&Passives Panel"
"Passzí&v panel"
"Panel &pasywny"
"Panel &pasivo"
"&Пассивна панель"

MacroSettingsPluginPanel
"На панели пла&гина"
"P&lugin panel"
"Panel p&luginů"
"P&lugin Panel"
"Ha &plugin panel"
"Panel p&luginów"
"Panel de p&lugins"
"На панелі пла&гіна"

MacroSettingsFolders
"Выполнять для папо&к"
"Execute for &folders"
"Spustit pro ad&resáře"
"Auf Ordnern aus&führen"
"Ha &mappa"
"Wykonaj dla &folderów"
"Ejecutar para &directorios"
"Виконувати для те&к"

MacroSettingsSelectionPresent
"&Отмечены файлы"
"Se&lection present"
"E&xistující výběr"
"Auswah&l vorhanden"
"Ha van ki&jelölés"
"Zaznaczenie &obecne"
"Selección presente"
"&Визначено файли"

MacroSettingsCommandLine
"Пустая командная &строка"
"Empty &command line"
"Prázdný pří&kazový řádek"
"Leere Befehls&zeile"
"Ha &üres a parancssor"
"Pusta &linia poleceń"
"Vaciar línea de &comandos"
"Порожній командний &рядок"

MacroSettingsSelectionBlockPresent
"Отмечен б&лок"
"Selection &block present"
"Existující blok výběr&u"
"Mar&kierter Text vorhanden"
"Ha van kijelölt &blokk"
"Obecny &blok zaznaczenia"
"Selección de bloque presente"
"Відзначений б&лок"

CannotSaveFile
l:
"Ошибка сохранения файла"
"Cannot save file"
"Nelze uložit soubor"
"Kann Datei nicht speichern"
"A fájl nem menthető"
"Nie mogę zapisać pliku"
"No se puede guardar archivo"
"Помилка збереження файлу"

TextSavedToTemp
"Отредактированный текст записан в"
"Edited text is stored in"
"Editovaný text je uložen v"
"Editierter Text ist gespeichert in"
"A szerkesztett szöveg elmentve:"
"Edytowany tekst został zachowany w"
"Texto editado es almacenado en"
"Відредагований текст записано в"

MonthJan
l:
"Янв"
"Jan"
"Led"
"Jan"
"Jan"
"Sty"
"Ene"
"Січ"

HelpHotKey
"Введите горячую клавишу (букву или цифру)"
"Enter hot key (letter or digit)"
"Zadejte horkou klávesu (písmeno nebo číslici)"
"Buchstabe oder Ziffer:"
"Nyomja le a billentyűt (betű vagy szám)"
"Podaj klawisz skrótu (litera lub cyfra)"
"Entrar tecla rápida (letra o dígito)"
"Введіть гарячу клавішу (літеру або цифру)"

PluginHotKeyBottom
"F4: горячая клавиша, F3: информация"
"F4: set hot key, F3: information"
"F4: nastavení horké klávesy, F3: informace"
"Kurztaste setzen: F4, information: F3"
"F4: gyorsbillentyű hozzárendelés, F3: information"
"F4: ustaw klawisz skrótu, F3: informacja"
"F4: asignar tecla rápida, F3: información"
"F4: гаряча клавіша, F3: інформація"

PluginHotKeyTitle
l:
"Назначение горячей клавиши"
"Assign plugin hot key"
"Přidělit horkou klávesu pluginu"
"Dem Plugin eine Kurztaste zuweisen"
"Plugin gyorsbillentyű hozzárendelés"
"Przypisz klawisz skrótu do pluginu"
"Asignar tecla rápida a plugin"
"Призначення гарячої клавіші"

LocationHotKeyTitle
l:
"Назначение горячей клавиши"
"Assign location hot key"
upd:"Přidělit horkou klávesu ???"
upd:"Dem ??? eine Kurztaste zuweisen"
upd:"??? gyorsbillentyű hozzárendelés"
upd:"Przypisz klawisz skrótu do ???"
upd:"Asignar tecla rápida a ???"
"Призначення гарячої клавіші"

RightCtrl
l:
"ПравыйCtrl"
"RightCtrl"
"PravýCtrl"
"StrgRechts"
"JobbCtrl"
"PrawyCtrl"
"CtrlDrcho"
"ПравиййCtrl"

ViewerGoTo
l:
"Перейти"
"Go to"
"Jdi na"
"Gehe zu"
"Ugrás"
"Idź do"
"Ir a:"
"Перейти"

GoToPercent
"&Процент"
"&Percent"
"&Procent"
"&Prozent"
"&Százalékban"
"&Procent"
"&Porcentaje"
"&Процент"

GoToHex
"16-ричное &смещение"
"&Hex offset"
"&Hex offset"
"Position (&Hex)"
"&Hexában"
"Pozycja (&szesnastkowo)"
"Dirección &Hexa"
"16-річне &зміщення"

GoToDecimal
"10-ичное с&мещение"
"&Decimal offset"
"&Desítkový offset"
"Position (&dezimal)"
"&Decimálisan"
"Pozycja (&dziesiętnie)"
"Dirección &Decimal"
"10-ічне з&міщення"

NetUserName
l:
"Имя пользователя"
"User name"
"Jméno uživatele"
"Benutzername"
"Felhasználói név"
"Nazwa użytkownika"
"Nombre de usuario"
"Ім'я користувача"

NetUserPassword
"Пароль пользователя"
"User password"
"Heslo uživatele"
"Benutzerpasswort"
"Felhasználói jelszó"
"Hasło użytkownika"
"Clave de usuario"
"Пароль користувача"

ReadFolderError
l:
"Не удаётся прочесть содержимое папки"
"Cannot read folder contents"
"Nelze načíst obsah adresáře"
"Kann Ordnerinhalt nicht lesen"
"A mappa tartalma nem olvasható"
"Nie udało się odczytać zawartości folderu"
"No se puede leer contenidos de directorios"
"Не вдається прочитати вміст теки"

PlgBadVers
l:
"Этот модуль требует FAR более высокой версии"
"This plugin requires higher FAR version"
"Tento plugin vyžaduje vyšší verzi FARu"
"Das Plugin benötigt eine aktuellere Version von FAR"
"A pluginhez újabb FAR verzió kell"
"Do uruchomienia pluginu wymagana jest wyższa wersja FAR-a"
"Este plugin requiere versión más actual de FAR"
"Цей модуль потребує FAR більш високої версії"

PlgRequired
"Требуется версия FAR - %d.%d"
"Required FAR version is %d.%d"
"Požadovaná verze FARu je %d.%d"
"Benötigte FAR-Version ist %d.%d"
"A szükséges FAR verzió: %d.%d"
"Wymagana wersja FAR-a to %d.%d"
"Requiere la versión FAR %d.%d"
"Потрібна версія FAR - %d.%d"

PlgRequired2
"Текущая версия FAR - %d.%d"
"Current FAR version is %d.%d"
"Nynější verze FARu je %d.%d"
"Aktuelle FAR-Version ist %d.%d"
"A jelenlegi FAR verzió: %d.%d"
"Bieżąca wersja FAR-a: %d.%d"
"Versión actual de FAR es %d.%d"
"Поточна версія FAR - %d.%d"

PlgLoadPluginError
"Ошибка при загрузке плагина"
"Error loading plugin module"
"Chyba při nahrávání zásuvného modulu"
"Fehler beim Laden des Pluginmoduls"
"Plugin betöltési hiba"
"Błąd ładowania modułu plugina"
"Error cargando módulo plugin"
"Помилка під час завантаження плагіна"

CheckBox2State
l:
"?"
"?"
"?"
"?"
"?"
"?"
"?"
"?"

HelpTitle
l:
"Помощь"
"Help"
"Nápověda"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

CannotOpenHelp
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie można otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

HelpTopicNotFound
"Не найден запрошенный раздел помощи:"
"Requested help topic not found:"
"požadované téma nápovědy nebylo nalezeno"
"Angefordertes Hilfethema wurde nicht gefunden:"
"A kívánt súgó témakör nem található:"
"Nie znaleziono tematu pomocy:"
"Tema de ayuda requerido no encontrado"
"Не знайдено запитаний розділ допомоги:"

PluginsHelpTitle
l:
"Внешние модули"
"Plugins help"
"Nápověda Pluginů"
"Pluginhilfe"
"Pluginek súgói"
"Pomoc dla pluginów"
"Ayuda plugins"
"Зовнішні модулі"

HelpSearchTitle
l:
"Поиск"
"Search"
"Hledání"
"Suchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

HelpSearchingFor
"Поиск для"
"Searching for"
"Hledání"
"Suche nach"
"Keresés:"
"Znajdź"
"Buscando por"
"Пошук для"

HelpF1
l:
l:// Help KeyBar F1-12
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

HelpShiftF1
l:
l:// Help KeyBar Shift-F1-12
"Содерж"
"Index"
"Index"
"Index"
"Tartlm"
"Indeks"
"Indice"
"Зміст"

HelpAltF1
l:
l:// Help KeyBar Alt-F1-12
"Пред."
"Prev"
"Předch"
"Letzt"
"Vissza"
"Poprz."
"Previo"
"Попрд."

HelpCtrlF1
l:
l:// Help KeyBar Ctrl-F1-12
""
""
""
""
""
""
""
""

HelpCtrlShiftF1
l:
l:// Help KeyBar CtrlShiftF1-12
""
""
""
""
""
""
""
""

HelpCtrlAltF1
l:
l:// Help KeyBar CtrlAltF1-12
""
""
""
""
""
""
""
""

HelpAltShiftF1
l:
l:// Help KeyBar AltShiftF1-12
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF1
l:
l:// Help KeyBar CtrlAltShiftF1-12
""
""
""
""
""
""
""
""

InfoF1
l:
l:// InfoPanel KeyBar F1-F12
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

InfoF3
"СмОпис"
"VieDiz"
"Zobraz"
"BetDiz"
"MjMnéz"
"VieDiz"
"VerDiz"
"ДвОпис"

InfoF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Win"
"ANSI"

InfoShiftF1
l:
l:// InfoPanel KeyBar Shift-F1-F12
""
""
""
""
""
""
""
""

InfoAltF1
l:
l:// InfoPanel KeyBar Alt-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

InfoCtrlF1
l:
l:// InfoPanel KeyBar Ctrl-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

InfoCtrlShiftF1
l:
l:// InfoPanel KeyBar CtrlShiftF1-12
""
""
""
""
""
""
""
""

InfoCtrlAltF1
l:
l:// InfoPanel KeyBar CtrlAltF1-12
""
""
""
""
""
""
""
""

InfoAltShiftF1
l:
l:// InfoPanel KeyBar AltShiftF1-12
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF1
l:
l:// InfoPanel KeyBar CtrlAltShiftF1-12
""
""
""
""
""
""
""
""

QViewF1
l:
l:// QView KeyBar F1-F12
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

QViewF4
"Код"
"Hex"
"Hex"
"Hex"
"Hexa"
"Hex"
"Hexa"
"Код"

QViewF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Win"
"ANSI"

QViewShiftF1
l:
l:// QView KeyBar Shift-F1-F12
""
""
""
""
""
""
""
""

QViewAltF1
l:
l:// QView KeyBar Alt-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

QViewCtrlF1
l:
l:// QView KeyBar Ctrl-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

QViewCtrlShiftF1
l:
l:// QView KeyBar CtrlShiftF1-12
""
""
""
""
""
""
""
""

QViewCtrlAltF1
l:
l:// QView KeyBar CtrlAltF1-12
""
""
""
""
""
""
""
""

QViewAltShiftF1
l:
l:// QView KeyBar AltShiftF1-12
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF1
l:
l:// QView KeyBar CtrlAltShiftF1-12
""
""
""
""
""
""
""
""

KBTreeF1
l:
l:// Tree KeyBar F1-F12
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

KBTreeShiftF1
l:
l:// Tree KeyBar Shift-F1-F12
""
""
""
""
""
""
""
""

KBTreeAltF1
l:
l:// Tree KeyBar Alt-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

KBTreeCtrlF1
l:
l:// Tree KeyBar Ctrl-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

KBTreeCtrlShiftF1
l:
l:// Tree KeyBar CtrlShiftF1-12
""
""
""
""
""
""
""
""

KBTreeCtrlAltF1
l:
l:// Tree KeyBar CtrlAltF1-12
""
""
""
""
""
""
""
""

KBTreeAltShiftF1
l:
l:// Tree KeyBar AltShiftF1-12
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF1
l:
l:// Tree KeyBar CtrlAltShiftF1-12
""
""
""
""
""
""
""
""

CopyTimeInfo
l:
"Время: %8.8ls    Осталось: %8.8ls    %8.8lsб/с"
"Time: %8.8ls    Remaining: %8.8ls    %8.8lsb/s"
"Čas: %8.8ls      Zbývá: %8.8ls      %8.8lsb/s"
"Zeit: %8.8ls   Verbleibend: %8.8ls   %8.8lsb/s"
"Eltelt: %8.8ls    Maradt: %8.8ls    %8.8lsb/s"
"Czas: %8.8ls    Pozostało: %8.8ls    %8.8lsb/s"
"Tiempo: %8.8ls    Restante: %8.8ls    %8.8lsb/s"
"Час: %8.8ls    Залишилось: %8.8ls    %8.8lsб/с"

KeyESCWasPressed
l:
"Действие было прервано"
"Operation has been interrupted"
"Operace byla přerušena"
"Vorgang wurde unterbrochen"
"A műveletet megszakította"
"Operacja została przerwana"
"Operación ha sido interrumpida"
"Дія була перервана"

DoYouWantToStopWork
"Вы действительно хотите отменить действие?"
"Do you really want to cancel it?"
"Opravdu chcete operaci stornovat?"
"Wollen Sie den Vorgang wirklich abbrechen?"
"Valóban le akarja állítani?"
"Czy naprawdę chcesz ją anulować?"
"Desea realmente cancelar la operación?"
"Ви дійсно хочете скасувати дію?"

DoYouWantToStopWork2
"Продолжить выполнение?"
"Continue work? "
"Pokračovat v práci?"
"Vorgang fortsetzen? "
"Folytatja?"
"Kontynuować? "
"Continuar trabajo? "
"Продовжити виконання?"

CheckingFileInPlugin
l:
"Файл проверяется в плагине"
"The file is being checked by the plugin"
"Soubor je právě kontrolován pluginem"
"Datei wird von Plugin überprüft"
"A fájlt ez a plugin használja:"
"Plugin sprawdza plik"
"El archivo está siendo chequeado por el plugin"
"Файл перевіряється у плагіні"

DialogType
l:
"Диалог"
"Dialog"
"Dialog"
"Dialog"
"Párbeszéd"
"Dialog"
"Diálogo"
"Діалог"

HelpType
"Помощь"
"Help"
"Nápověda"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

FolderTreeType
"ПоискКаталогов"
"FolderTree"
"StromAdresáře"
"Ordnerbaum"
"MappaFa"
"Drzewo folderów"
"ArbolDirectorio"
"ПошукКаталогів"

VMenuType
"Меню"
"Menu"
"Menu"
"Menü"
"Menü"
"Menu"
"Menú"
"Меню"

IncorrectMask
l:
"Некорректная маска файлов"
"File-mask string contains errors"
"Řetězec masky souboru obsahuje chyby"
"Zeichenkette mit Dateimaske enthält Fehler"
"A fájlmaszk hibás"
"Maska pliku zawiera błędy"
"Cadena de máscara de archivos contiene errores"
"Неправильна маска файлів"

PanelBracketsForLongName
l:
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"

OpenPluginCannotOpenFile
l:
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie można otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

FileFilterTitle
l:
"Фильтр"
"Filter"
"Filtr"
"Filter"
"Felhasználói szűrő"
"Filtr wyszukiwania"
"Filtro"
"Фільтр"

FileHilightTitle
"Раскраска файлов"
"Files highlighting"
"Zvýrazňování souborů"
"Farbmarkierungen"
"Fájlkiemelés"
"Zaznaczanie plików"
"Resaltado de archivos"
"Розмальовка файлів"

FileFilterName
"Имя &фильтра:"
"Filter &name:"
"Jmé&no filtru:"
"Filter&name:"
"Szűrő &neve:"
"Nazwa &filtra:"
"&Nombre filtro:"
"Ім'я &фільтра:"

FileFilterMatchMask
"&Маска:"
"&Mask:"
"&Maska"
"&Maske:"
"&Maszk:"
"&Maska:"
"&Máscara:"
"&Маска:"

FileFilterSize
"Разм&ер:"
"Si&ze:"
"Vel&ikost"
"G&röße:"
"M&éret:"
"Ro&zmiar:"
"&Tamaño:"
"Розм&ір:"

FileFilterSizeFromSign
">="
">="
">="
">="
">="
">="
">="
">="

FileFilterSizeToSign
"<="
"<="
"<="
"<="
"<="
"<="
"<="
"<="

FileFilterDate
"&Дата/Время:"
"Da&te/Time:"
"Dat&um/Čas:"
"Da&tum/Zeit:"
"&Dátum/Idő:"
"Da&ta/Czas:"
"&Fecha/Hora:"
"&Дата/Час:"

FileFilterWrited
"&записи"
upd:"&write"
upd:"&write"
upd:"&write"
upd:"&write"
upd:"&write"
"&modificación"
"&записи"

FileFilterDateRelative
"Относительна&я"
"Relat&ive"
"Relati&vní"
"Relat&iv"
"Relat&ív"
"Relat&ive"
"Relat&ivo"
"Відносн&а"

FileFilterDateAfterSign
">="
">="
">="
">="
">="
">="
">="
">="

FileFilterDateBeforeSign
"<="
"<="
"<="
"<="
"<="
"<="
"<="
"<="

FileFilterCurrent
"Теку&щая"
"C&urrent"
"Aktuá&lní"
"Akt&uell"
"&Jelenlegi"
"&Bieżący"
"Act&ual"
"Пото&чна"

FileFilterBlank
"С&брос"
"B&lank"
"Práz&dný"
"&Leer"
"&Üres"
"&Wyczyść"
"En b&lanco"
"С&кидання"

FileFilterAttr
"Атрибут&ы"
"Attri&butes"
"Attri&buty"
"Attri&bute"
"Attri&bútumok"
"&Atrybuty"
"Atri&butos"
"Атрибут&и"

FileFilterAttrR
"&Только для чтения"
"&Read only"
"Jen pro čt&ení"
"Sch&reibschutz"
"&Csak olvasható"
"&Do odczytu"
"Sólo Lectu&ra"
"&Тільки для читання"

FileFilterAttrA
"&Архивный"
"&Archive"
"Arc&hivovat"
"&Archiv"
"&Archív"
"&Archiwalny"
"&Archivo"
"&Архівний"

FileFilterAttrH
"&Скрытый"
"&Hidden"
"Skry&tý"
"&Versteckt"
"&Rejtett"
"&Ukryty"
"&Oculto"
"&Скритий"

FileFilterAttrS
"С&истемный"
"&System"
"Systémo&vý"
"&System"
"Re&ndszer"
"&Systemowy"
"&Sistema"
"С&истемний"

FileFilterAttrC
"С&жатый"
"&Compressed"
"Kompri&movaný"
"&Komprimiert"
"&Tömörített"
"S&kompresowany"
"&Comprimido"
"С&тиснутий"

FileFilterAttrE
"&Зашифрованный"
"&Encrypted"
"Ši&frovaný"
"V&erschlüsselt"
"T&itkosított"
"&Zaszyfrowany"
"Ci&frado"
"&Зашифрований"

FileFilterAttrD
"&Каталог"
"&Directory"
"Adr&esář"
"Ver&zeichnis"
"Map&pa"
"&Katalog"
"&Directorio"
"&Каталог"

FileFilterAttrNI
"&Неиндексируемый"
"Not inde&xed"
"Neinde&xovaný"
"Nicht in&diziert"
"Nem inde&xelt"
"Nie z&indeksowany"
"No inde&xado"
"&Неіндексований"

FileFilterAttrSparse
"&Разрежённый"
"S&parse"
"Říd&ký"
"Reserve"
"Ritk&ított"
"S&parse"
"Escaso"
"&Розріджений"

FileFilterAttrT
"&Временный"
"Temporar&y"
"Doča&sný"
"Temporär"
"Átm&eneti"
"&Tymczasowy"
"Tempora&l"
"&Тимчасовий"

FileFilterAttrReparse
"Симво&л. ссылка"
"Symbolic lin&k"
"Sybolický li&nk"
"Symbolischer Lin&k"
"S&zimbolikus link"
"Link &symboliczny"
"Enlace simbólic&o"
"Симво&л. посилання"

FileFilterAttrOffline
"Автономны&й"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"Автономни&й"

FileFilterAttrVirtual
"Вирт&уальный"
"&Virtual"
"Virtuální"
"&Virtuell"
"&Virtuális"
"&Wirtualny"
"&Virtual"
"Вірт&уальний"

FileFilterAttrExecutable
"Исполняемый"
"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
"Виконуваний"

FileFilterAttrBroken
"Неисправный"
"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
"Несправний"

FileFilterReset
"Очистит&ь"
"Reset"
"Reset"
"Rücksetzen"
"Reset"
"Wy&czyść"
"Reinicio"
"Очистит&и"

FileFilterCancel
"Отмена"
"Cancel"
"Storno"
"Abbruch"
"Mégsem"
"&Anuluj"
"Cancelar"
"Відміна"

FileFilterMakeTransparent
"Выставить прозрачность"
"Make transparent"
"Zprůhlednit"
"Transparent"
"Legyen átlátszó"
"Ustaw jako przezroczysty"
"Hacer transparente"
"Виставити прозорість"

BadFileSizeFormat
"Неправильно заполнено поле размера"
"File size field is incorrectly filled"
"Velikost souboru neobsahuje správnou hodnotu"
"Angabe der Dateigröße ist fehlerhaft"
"A fájlméret mező rosszul van kitöltve"
"Rozmiar pliku jest niepoprawny"
"Campo de tamaño de archivo no está correctamente llenado"
"Неправильно заповнено поле розміру"

FarTitleAddonsAdmin
l:
"root"
"root"
upd:"root"
upd:"root"
upd:"root"
upd:"root"
"root"
"root"

TerminalClipboardAccessTitle
"Доступ к буферу обмена"
"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
"Доступ до буфера обміну"

TerminalClipboardSetText
"Приложение хочет записать в буфер обмена."
"Application wants to set clipboard data."
upd:"Application wants to set clipboard data."
upd:"Application wants to set clipboard data."
upd:"Application wants to set clipboard data."
upd:"Application wants to set clipboard data."
upd:"Application wants to set clipboard data."
upd:"Application wants to set clipboard data."

TerminalClipboardSetAllowOnce
"&Разрешить однократно"
"&Allow once"
upd:"&Allow once"
upd:"&Allow once"
upd:"&Allow once"
upd:"&Allow once"
upd:"&Allow once"
upd:"&Allow once"

TerminalClipboardSetAllowForCommand
"Разрешить &этой команде"
"Allow for &this command"
upd:"Allow for &this command"
upd:"Allow for &this command"
upd:"Allow for &this command"
upd:"Allow for &this command"
upd:"Allow for &this command"
upd:"Allow for &this command"

TerminalClipboardAccessText
"Укажите как это приложение может пользоваться буфером обмена."
"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
"Вкажіть, як ця програма може користуватися буфером обміну."

TerminalClipboardAccessBlock
"&Заблокировать"
"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
"&Заблокувати"

TerminalClipboardAccessTemporaryRemote
"&Удаленный буфер"
"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
"&Віддалений буфер"

TerminalClipboardAccessTemporaryLocal
"&Общий буфер"
"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
"&Спільний буфер"

TerminalClipboardAccessAlwaysLocal
"Общий буфер всег&да"
"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
"Загальний буфер завж&ди"

MountsRoot
"Корень"
"Root"
upd:"Root"
upd:"Root"
upd:"Root"
upd:"Root"
upd:"Root"
"Корінь"

MountsHome
"Дом"
"Home"
upd:"Home"
upd:"Home"
upd:"Home"
upd:"Home"
upd:"Home"
"Дом"

MountsOther
"Др. панель"
"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
"Інш. панель"

MacroPluginLuamacroNotLoaded
"Плагин LuaMacro не загружен"
"Plugin LuaMacro is not loaded"
"Plugin LuaMacro není nahrán"
"Plugin LuaMacro wurde nicht geladen"
upd:"Plugin LuaMacro is not loaded"
"Wtyczka LuaMacro nie została wczytana"
"Complemento LuaMacro no está cargado"
"Плагін LuaMacro не завантажено"

MacroRecordingIsDisabled
"Запись макросов запрещена"
"Macro recording is disabled"
"Nahrávání maker je vypnuto"
"Macro Screiben is verhindert"
upd:"Macro recording is disabled"
"Nagrywanie makr jest wyłączone"
"Grabación de macro está desactivada"
"Запис макросів заборонено"

MPluginInformation
"Информация о плагине"
"Plugin information"
"Informace o modulu"
"Information über Plugin"
upd:"Plugin information"
"Informacje o wtyczce"
"Información de complemento"
"Інформація про плагін"

MPluginModuleTitle
"&Название:"
"&Title:"
"&Nadpis:"
"&Titel:"
upd:"&Title:"
"&Tytuł:"
"&Título:"
"&Назва"

MPluginDescription
"&Описание:"
"&Description:"
"&Popis:"
"&Beschreibung:"
upd:"&Description:"
"&Opis:"
"&Descripción:"
"&Опис:"

MPluginAuthor
"&Автор:"
"&Author:"
"&Autor:"
"&Autor:"
upd:"&Author:"
"&Autor:"
"&Autor:"
"&Автор:"

MPluginVersion
"&Версия:"
"&Version:"
"&Verze:"
upd:"&Version:"
upd:"&Version:"
"&Wersja:"
"&Versión:"
"&Версія:"

MPluginModulePath
"&Файл плагина:"
"&Module path:"
"Cesta k &modulu:"
"&Plugin-Datei:"
upd:"&Module path:"
"Ścieżka &modułu:"
"Ruta de &módulo:"
"&Файл плагіна:"

MPluginSysID
"&SysID плагина:"
"Plugin &SysID:"
"&SysID modulu:"
"Plugin &SysID"
upd:"Plugin &SysID:"
"&SysID wtyczki:"
"&SysID complemento:"
"&SysID плагіна:"

MPluginPrefix
"&Префикс плагина:"
"Plugin &prefix:"
"Předpona mo&dulu:"
"Plugin &Präfix:"
upd:"Plugin &prefix:"
"&Przedrostek wtyczki:"
"&Prefijo de complemento:"
"&Префікс плагіна:"

MDataNotAvailable
"(нет данных)"
"(data not available)"
"(data nejsou k dispozici)"
"(nicht verfügbar)"
"(adat nem elérhető)"
"(dane niedostępne)"
"(dato no disponible)"
"(немає даних)"

#Must be the last
NewFileName
l:
"?Новый файл?"
"?New File?"
"?Nový soubor?"
"?Neue Datei?"
"?Új fájl?"
"?Nowy plik?"
"?Nuevoi Archivo?"
"?Новий файл?"

