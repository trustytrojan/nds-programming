#include "everything.h"

int main2(void)
{
	// resource initialization
	createTerminal();
	Wifi_InitDefault(false);

	// start shell prompt
	std::string line;
	while (1)
	{
		std::cout << "> ";
		std::getline(std::cin, line);

		const auto args = strsplit(line, ' ');
		const auto numArgs = args.size();
		if (!numArgs)
			continue;

		const auto command = args[0];
		if (command.empty())
			continue;

		if (command == "exit")
			break;

		if (command == "help")
			std::cout << "commands: help exit dns tcp wifi\n";

		else if (command == "dns")
		{
			if (numArgs == 1)
			{
				std::cout << "usage: dns <hostname>\n";
				continue;
			}

			const auto host = gethostbyname(args[1].c_str());
			if (!host)
			{
				perror("gethostbyname");
				continue;
			}

			std::cout << inet_ntoa(*(in_addr *)host->h_addr_list[0]) << '\n';
		}

		else if (command == "tcp")
		{
			if (numArgs == 1)
			{
				std::cout << "usage: tcp <ipaddr>\n";
				continue;
			}

			const auto sain = parseAddress(args[1], -1);
			if (!sain)
			{
				npPrintError();
				continue;
			}

			const auto sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == -1)
			{
				perror("socket");
				continue;
			}

			std::cout << "Created socket\n";
			
			if (connect(sock, (sockaddr *)sain, sizeof(sockaddr_in)) == -1)
			{
				perror("connect");
				continue;
			}

			delete sain;
			std::cout << "Connected to " << args[1] << '\n';

			tcpClient(sock);

			close(sock);
			std::cout << "Closed socket\n";
		}

		else if (command == "http")
		{
			if (numArgs == 1)
			{
				std::cout << "usage: http <url>\n";
				continue;
			}

			const auto url = args[1];
		}

		else if (command == "wifi")
		{
			if (numArgs == 1)
			{
				std::cout << "usage: wifi <subcommand>\nsubcommands: connect, status\n";
				continue;
			}

			const auto subcommand = args[1];

			if (subcommand == "connect")
			{
				const auto ap = findAP();
				consoleClear();
				Wifi_SetIP(0, 0, 0, 0, 0);

				if (ap->flags & WFLAG_APDATA_WEP)
				{
					std::cout << "Password: ";
					std::string password;

					std::getline(std::cin, password);
					auto wepmode = WEPMODE_NONE;
					if (password.size() == 5)
						wepmode = WEPMODE_40BIT;
					else if (password.size() == 13)
						wepmode = WEPMODE_128BIT;
					else
					{
						std::cout << "Invalid password!\n";
						continue;
					}
					Wifi_ConnectAP(ap, wepmode, 0, (unsigned char *)password.c_str());
				}
				else
				{
					Wifi_ConnectAP(ap, WEPMODE_NONE, 0, 0);
				}

				int curr = Wifi_AssocStatus(), prev = -1;
				while (curr != ASSOCSTATUS_ASSOCIATED && curr != ASSOCSTATUS_CANNOTCONNECT)
				{
					if (curr != prev)
						PrintWifiStatus();
					prev = curr;
					curr = Wifi_AssocStatus();
				}
				PrintWifiStatus();
			}

			else if (subcommand == "status")
			{
				PrintWifiStatus();
			}
		}

		else
		{
			std::cout << "unknown command\n";
		}
	}
}
