#include "../Shell.hpp"

using namespace Shell;

// from ap_search.cpp
void FindAPInteractive(Wifi_AccessPoint &ap);

// for printing ip addresses without first converting to a string
std::ostream &operator<<(std::ostream &ostr, const in_addr &ip)
{
	return ostr << (ip.s_addr & 0xFF) << '.' << ((ip.s_addr >> 8) & 0xFF) << '.' << ((ip.s_addr >> 16) & 0xFF) << '.' << ((ip.s_addr >> 24) & 0xFF);
}

static const char *wifiStatusStr[] = {"Disconnected", "Searching", "Authenticating", "Associating", "Acquiring DHCP", "Associated", "Cannot Connect"};
static const auto usageStr = "usage:\n"
							 "\twifi scan\n"
							 "\twifi list\n"
							 "\twifi con[nect] [ssid] [-q]\n"
							 "\twifi status\n"
							 "\twifi enable\n"
							 "\twifi disable\n"
							 "\twifi ip\n"
							 "\twifi ipinfo\n";

bool GetPassword(std::string &password, WEPMODES &wepmode)
{
	std::cout << "Password: ";
	std::getline(std::cin, password); // characters are not shown since keyboard->OnKeyPressed is NULL
	switch (password.size())
	{
	case 5:
		wepmode = WEPMODE_40BIT;
		break;
	case 13:
		wepmode = WEPMODE_128BIT;
		break;
	default:
		std::cout << "\e[41minvalid password\e[39m\n";
		return false;
	}
	return true;
}

bool FindAPWithSSID(const std::string &ssid, Wifi_AccessPoint &ap)
{
	if (ssid.empty())
	{
		std::cerr << "\e[41mssid cannot be empty\e[39m\n";
		return false;
	}

	// loop through and find the one with our ssid
	const auto numAPs = Wifi_GetNumAP();
	int i = 0;
	for (; i < numAPs; ++i)
	{
		Wifi_GetAPData(i, &ap);
		if (ap.ssid_len && ap.ssid == ssid)
			return true;
	}

	std::cerr << "\e[41mssid '" << ssid << "' not found\e[39m\ncheck output of `wifi list`\n";
	return false;
}

void connect()
{
	Wifi_AccessPoint ap;

	if (args.size() == 2)
	{
		FindAPInteractive(ap);
		consoleClear();
	}
	else
		FindAPWithSSID(args[2], ap);

	// Use DHCP
	Wifi_SetIP(0, 0, 0, 0, 0);

	// Authenticate if needed and connect
	if (ap.flags & WFLAG_APDATA_WEP)
	{
		std::string password;
		WEPMODES wepmode;
		if (!GetPassword(password, wepmode))
			return;
		Wifi_ConnectAP(&ap, wepmode, 0, (unsigned char *)password.c_str());
	}
	else
		Wifi_ConnectAP(&ap, WEPMODE_NONE, 0, 0);

	// wait until associated or cannot connect
	// print each status unless -q was passed
	auto status = Wifi_AssocStatus(), prevStatus = -1;
	const auto quiet = (std::ranges::find(args, "-q") != args.end());
	for (; status != ASSOCSTATUS_ASSOCIATED && status != ASSOCSTATUS_CANNOTCONNECT; status = Wifi_AssocStatus())
	{
		if (!quiet && status != prevStatus)
		{
			std::cout << wifiStatusStr[status] << '\n';
			prevStatus = status;
		}
		swiWaitForVBlank();
	}

	if (status == ASSOCSTATUS_CANNOTCONNECT)
		std::cout << "\e[41mCannot connect to '" << ap.ssid << "'\e[39m\n";
	else
		std::cout << "Connected to '" << ap.ssid << "'\n";
}

void list()
{
	const auto numAPs = Wifi_GetNumAP();

	if (!numAPs)
	{
		std::cout << "no APs found\ndid you run `wifi scan`?\n";
		return;
	}

	// eventually parameterize this with cmdline options
	const auto numToPrint = std::min(numAPs, 15);

	// Populate a vector with the AP data
	std::vector<Wifi_AccessPoint> aps(numAPs);
	for (auto i = 0; i < numAPs; ++i)
		Wifi_GetAPData(i, &aps[i]);

	// Sort the vector by AP signal
	std::sort(aps.begin(), aps.end(), [](const auto &ap1, const auto &ap2)
			  { return ap1.rssi > ap2.rssi; });

	// Print the top 15 or less results
	std::cout << "Sig Sec  SSID\n";
	std::ranges::for_each_n(aps.cbegin(), numToPrint, [](const auto &ap)
							{ std::cout << std::setw(3) << (ap.rssi * 100 / 0xD0) << ' ' << std::setw(4) << ((ap.flags & WFLAG_APDATA_WEP) ? "WEP" : "Open") << " '" << ap.ssid << "'\n"; });
}

void ipinfo()
{
	in_addr gateway, subnetMask, dns1, dns2;
	Wifi_GetIPInfo(&gateway, &subnetMask, &dns1, &dns2);
	std::cout << "Gateway:    " << gateway << "\nSubnet Mask: " << subnetMask << "\nDNS 1:       " << dns1 << "\nDNS 2:       " << dns2 << '\n';
}

void Commands::wifi()
{
	if (args.size() == 1)
	{
		std::cerr << usageStr;
		return;
	}

	const auto &subcommand = args[1];

	if (!strncmp(subcommand.c_str(), "con", 3))
		connect();

	else if (subcommand == "scan")
		Wifi_ScanMode();

	else if (subcommand == "list")
		list();

	else if (subcommand == "status")
		std::cout << wifiStatusStr[Wifi_AssocStatus()] << '\n';

	else if (subcommand == "disconnect")
		std::cout << Wifi_DisconnectAP() << '\n';

	else if (subcommand == "disable")
		Wifi_DisableWifi();

	else if (subcommand == "enable")
		Wifi_EnableWifi();

	else if (subcommand == "ip")
		std::cout << (in_addr)Wifi_GetIP() << '\n';

	else if (subcommand == "ipinfo")
		ipinfo();

	else
		std::cerr << usageStr;
}