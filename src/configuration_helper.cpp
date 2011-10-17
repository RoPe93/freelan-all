/*
 * freelan - An open, multi-platform software to establish peer-to-peer virtual
 * private networks.
 *
 * Copyright (C) 2010-2011 Julien KAUFFMANN <julien.kauffmann@freelan.org>
 *
 * This file is part of freelan.
 *
 * freelan is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * freelan is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 *
 * If you intend to use freelan in a commercial software, please
 * contact me : we may arrange this for a small fee or no fee at all,
 * depending on the nature of your project.
 */

/**
 * \file configuration_helper.cpp
 * \author Julien KAUFFMANN <julien.kauffmann@freelan.org>
 * \brief A configuration helper.
 */

#include "configuration_helper.hpp"

#include <vector>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "parsers.hpp"

namespace po = boost::program_options;
namespace fl = freelan;

namespace
{
	fl::configuration::hostname_resolution_protocol_type parse_network_hostname_resolution_protocol(const std::string& str)
	{
		if (str == "system_default")
			return boost::asio::ip::udp::v4();
		else if (str == "ipv4")
			return boost::asio::ip::udp::v4();
		else if (str == "ipv6")
			return boost::asio::ip::udp::v6();

		throw std::runtime_error("\"" + str + "\" is not a valid hostname resolution protocol");
	}

	fl::switch_configuration::routing_method_type to_routing_method(const std::string& str)
	{
		if (str == "switch")
			return fl::switch_configuration::RM_SWITCH;
		if (str == "hub")
			return fl::switch_configuration::RM_HUB;

		throw std::runtime_error("\"" + str + "\" is not a valid routing method");
	}

	fl::configuration::certificate_validation_method_type to_certificate_validation_method(const std::string& str)
	{
		if (str == "default")
			return fl::configuration::CVM_DEFAULT;
		if (str == "none")
			return fl::configuration::CVM_NONE;

		throw std::runtime_error("\"" + str + "\" is not a valid certificate validation method");
	}

	boost::posix_time::time_duration to_time_duration(unsigned int msduration)
	{
		return boost::posix_time::milliseconds(msduration);
	}

	cryptoplus::file load_file(const std::string& filename)
	{
		try
		{
			return cryptoplus::file::open(filename);
		}
		catch (std::runtime_error&)
		{
			throw std::runtime_error("Unable to open the specified file: " + filename);
		}
	}

	fl::configuration::cert_type load_certificate(const std::string& filename)
	{
		return fl::configuration::cert_type::from_certificate(load_file(filename));
	}

	cryptoplus::pkey::pkey load_private_key(const std::string& filename)
	{
		return cryptoplus::pkey::pkey::from_private_key(load_file(filename));
	}

	fl::configuration::cert_type load_trusted_certificate(const std::string& filename)
	{
		return fl::configuration::cert_type::from_trusted_certificate(load_file(filename));
	}
}

po::options_description get_network_options()
{
	po::options_description result("Network options");

	result.add_options()
		("network.hostname_resolution_protocol", po::value<std::string>()->default_value("system_default"), "The hostname resolution protocol to use.")
		("network.listen_on", po::value<std::string>()->default_value("0.0.0.0:12000"), "The endpoint to listen on.")
		("network.enable_tap_adapter", po::value<bool>()->default_value(true, "yes"), "Whether to enable the tap adapter.")
		("network.tap_adapter_ipv4_address_prefix_length", po::value<std::string>()->default_value("9.0.0.1/24"), "The tap adapter IPv4 address and prefix length.")
		("network.tap_adapter_ipv6_address_prefix_length", po::value<std::string>()->default_value("fe80::1/10"), "The tap adapter IPv6 address and prefix length.")
		("network.enable_arp_proxy", po::value<bool>()->default_value(false), "Whether to enable the ARP proxy.")
		("network.arp_proxy_fake_ethernet_address", po::value<std::string>()->default_value("00:aa:bb:cc:dd:ee"), "The ARP proxy fake ethernet address.")
		("network.enable_dhcp_proxy", po::value<bool>()->default_value(true), "Whether to enable the DHCP proxy.")
		("network.dhcp_server_ipv4_address_prefix_length", po::value<std::string>()->default_value("9.0.0.0/24"), "The DHCP proxy server IPv4 address and prefix length.")
		("network.dhcp_server_ipv6_address_prefix_length", po::value<std::string>()->default_value("fe80::/10"), "The DHCP proxy server IPv6 address and prefix length.")
		("network.hello_timeout", po::value<std::string>()->default_value("3000"), "The default hello message timeout, in milliseconds.")
		("network.contact", po::value<std::vector<std::string> >()->multitoken()->zero_tokens()->default_value(std::vector<std::string>(), ""), "The contact list.")
		;

	return result;
}

po::options_description get_switch_options()
{
	po::options_description result("Switch options");

	result.add_options()
		("switch.routing_method", po::value<std::string>()->default_value("switch"), "The routing method for messages.")
		("switch.enable_relay_mode", po::value<bool>()->default_value(false, "no"), "Whether to enable the relay mode.")
		("switch.enable_stp", po::value<bool>()->default_value(false, "no"), "Whether to enable the Spanning Tree Protocol.")
		;

	return result;
}

po::options_description get_security_options()
{
	po::options_description result("Security options");

	result.add_options()
		("security.signature_certificate_file", po::value<std::string>()->required(), "The certificate file to use for signing.")
		("security.signature_private_key_file", po::value<std::string>()->required(), "The private key file to use for signing.")
		("security.encryption_certificate_file", po::value<std::string>(), "The certificate file to use for encryption.")
		("security.encryption_private_key_file", po::value<std::string>(), "The private key file to use for encryption.")
		("security.certificate_validation_method", po::value<std::string>()->default_value("default"), "The certificate validation method.")
		("security.certificate_validation_script", po::value<std::string>(), "The certificate validation script to use.")
		("security.authority_certificate_file", po::value<std::vector<std::string> >()->multitoken()->zero_tokens()->default_value(std::vector<std::string>(), ""), "The authority certificates.")
		;

	return result;
}

void setup_configuration(fl::configuration& configuration, const po::variables_map& vm)
{
	typedef boost::asio::ip::udp::resolver::query query;
	typedef fl::configuration::cert_type cert_type;
	typedef cryptoplus::pkey::pkey pkey;

	// Network options
	configuration.hostname_resolution_protocol = parse_network_hostname_resolution_protocol(vm["network.hostname_resolution_protocol"].as<std::string>());
	configuration.listen_on = parse<fl::configuration::ep_type>(vm["network.listen_on"].as<std::string>());
	configuration.enable_tap_adapter = vm["network.enable_tap_adapter"].as<bool>();
	configuration.tap_adapter_ipv4_address_prefix_length = parse_optional<fl::configuration::ipv4_address_prefix_length_type>(vm["network.tap_adapter_ipv4_address_prefix_length"].as<std::string>());
	configuration.tap_adapter_ipv6_address_prefix_length = parse_optional<fl::configuration::ipv6_address_prefix_length_type>(vm["network.tap_adapter_ipv6_address_prefix_length"].as<std::string>());
	configuration.enable_arp_proxy = vm["network.enable_arp_proxy"].as<bool>();
	configuration.arp_proxy_fake_ethernet_address = parse<fl::configuration::ethernet_address_type>(vm["network.arp_proxy_fake_ethernet_address"].as<std::string>());
	configuration.enable_dhcp_proxy = vm["network.enable_dhcp_proxy"].as<bool>();
	configuration.dhcp_server_ipv4_address_prefix_length = parse_optional<fl::configuration::ipv4_address_prefix_length_type>(vm["network.dhcp_server_ipv4_address_prefix_length"].as<std::string>());
	configuration.dhcp_server_ipv6_address_prefix_length = parse_optional<fl::configuration::ipv6_address_prefix_length_type>(vm["network.dhcp_server_ipv6_address_prefix_length"].as<std::string>());
	configuration.hello_timeout = to_time_duration(boost::lexical_cast<unsigned int>(vm["network.hello_timeout"].as<std::string>()));

	const std::vector<std::string> contact_list = vm["network.contact"].as<std::vector<std::string> >();

	configuration.contact_list.clear();

	BOOST_FOREACH(const std::string& contact, contact_list)
	{
		configuration.contact_list.push_back(parse<fl::configuration::ep_type>(contact));
	}

	// Switch options
	configuration.switch_configuration.routing_method = to_routing_method(vm["switch.routing_method"].as<std::string>());
	configuration.switch_configuration.enable_relay_mode = vm["switch.enable_relay_mode"].as<bool>();
	configuration.switch_configuration.enable_stp = vm["switch.enable_stp"].as<bool>();

	// Security options
	cert_type signature_certificate = load_certificate(vm["security.signature_certificate_file"].as<std::string>());
	pkey signature_private_key = load_private_key(vm["security.signature_private_key_file"].as<std::string>());

	cert_type encryption_certificate;
	pkey encryption_private_key;

	if (vm.count("security.encryption_certificate_file"))
	{
		encryption_certificate = load_certificate(vm["security.encryption_certificate_file"].as<std::string>());
	}

	if (vm.count("security.encryption_private_key_file"))
	{
		encryption_private_key = load_private_key(vm["security.encryption_private_key_file"].as<std::string>());
	}

	configuration.identity = fscp::identity_store(signature_certificate, signature_private_key, encryption_certificate, encryption_private_key);

	configuration.certificate_validation_method = to_certificate_validation_method(vm["security.certificate_validation_method"].as<std::string>());

	const std::vector<std::string> authority_certificate_file_list = vm["security.authority_certificate_file"].as<std::vector<std::string> >();

	configuration.certificate_authorities.clear();

	BOOST_FOREACH(const std::string& authority_certificate_file, authority_certificate_file_list)
	{
		configuration.certificate_authorities.push_back(load_trusted_certificate(authority_certificate_file));
	}
}

std::string get_certificate_validation_script(const boost::program_options::variables_map& vm)
{
	return vm["security.certificate_validation_script"].as<std::string>();
}
