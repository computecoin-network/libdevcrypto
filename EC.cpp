/*
 This file is part of cpp-ethereum.

 cpp-ethereum is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 cpp-ethereum is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
 */
/** @file EC.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * Shared EC classes and functions.
 */

#pragma warning(push)
#pragma warning(disable:4100 4244)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wextra"
#include <files.h>
#pragma warning(pop)
#pragma GCC diagnostic pop
#include "CryptoPP.h"
#include "SHA3.h"
#include "EC.h"

// CryptoPP and dev conflict so dev and pp namespace are used explicitly
using namespace std;
using namespace dev;
using namespace dev::crypto;
using namespace CryptoPP;

void dev::crypto::encrypt(Public const& _key, bytes& _plain)
{
	ECIES<ECP>::Encryptor e;
	e.AccessKey().AccessGroupParameters().Initialize(pp::secp256k1());
	e.AccessKey().SetPublicElement(pp::PointFromPublic(_key));
	size_t plen = _plain.size();
	std::string c;
	c.resize(e.CiphertextLength(plen));
	e.Encrypt(pp::PRNG(), _plain.data(), plen, (byte*)c.data());
	_plain.resize(c.size());
	memcpy(_plain.data(), c.data(), c.size());
}

void dev::crypto::decrypt(Secret const& _k, bytes& _c)
{
	CryptoPP::ECIES<CryptoPP::ECP>::Decryptor d;
	d.AccessKey().AccessGroupParameters().Initialize(pp::secp256k1());
	d.AccessKey().SetPrivateExponent(pp::ExponentFromSecret(_k));
	size_t clen = _c.size();
	std::string p;
	p.resize(d.MaxPlaintextLength(_c.size()));
	DecodingResult r = d.Decrypt(pp::PRNG(), _c.data(), clen, (byte*)p.data());
	assert(r.messageLength);
	_c.resize(r.messageLength);
	memcpy(_c.data(), p.data(), _c.size());
}

SecretKeyRef::SecretKeyRef()
{
	secp256k1_start();
	static std::mt19937_64 s_eng(time(0));
	std::uniform_int_distribution<uint16_t> d(0, 255);

	for (int i = 0; i < 100; ++i)
	{
		for (unsigned i = 0; i < 32; ++i)
			m_secret[i] = (byte)d(s_eng);
		
		KeyPair ret(m_secret);
		if (ret.address())
			break;
	}
	/// todo: throw exception if key doesn't happen (or run forever?)
	/// todo: ^ also in KeyPair::create()
}

Public SecretKeyRef::pub() const
{
	Public p;
	pp::PublicFromExponent(pp::ExponentFromSecret(m_secret), p);
	return p;
}

Address SecretKeyRef::address() const
{
	return dev::right160(dev::sha3(bytesConstRef(pub().data(), 64)));
}

