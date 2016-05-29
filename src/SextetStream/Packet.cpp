
#include "SextetStream/Packet.h"
#include "RageLog.h"

typedef RString::value_type RChr;
typedef std::vector<RChr> RVector;
typedef SextetStream::Packet::ProcessEventCallback ProcessEventCallback;

namespace
{
	inline size_t max(size_t a, size_t b)
	{
		return (a > b) ? a : b;
	}

	inline size_t min(size_t a, size_t b)
	{
		return (a < b) ? a : b;
	}

	inline size_t min(size_t a, size_t b, size_t c)
	{
		return min(min(a, b), c);
	}

	inline RChr Armored(RChr x)
	{
		return ((x + 0x10) & 0x3F) + 0x30;
	}

	const RChr ARMORED_0 = Armored(0);

	inline bool IsArmored(RChr x)
	{
		return (x >= 0x30) && (x <= 0x6F);
	}

	inline size_t FindCleanPacketLeft(const RString& line)
	{
		size_t left = 0;
		size_t end = line.length();

		while(left < end) {
			if(IsArmored(line[left])) {
				break;
			}
			++left;
		}

		return left;
	}

	inline size_t FindCleanPacketRight(const RString& line, size_t left)
	{
		size_t right = left;
		size_t end = line.length();

		while(right < end) {
			if(!IsArmored(line[right])) {
				break;
			}
			++right;
		}

		return right;
	}

	// Does a = a ^ b for entire RVectors.
	inline void XorVectors(RVector& a, const RVector& b)
	{
		size_t alen = a.size();
		size_t blen = b.size();

		size_t len = min(alen, blen);

		// Only perform actual XOR on the common length.
		for(size_t i = 0; i < len; ++i) {
			a[i] = Armored(a[i] ^ b[i]);
		}

		// If this packet is shorter, the rest of the result is zero XOR b = b.
		// So, the rest is copied directly from b.
		if(alen < blen) {
			a.reserve(blen);
			for(size_t i = alen; i < blen; ++i) {
				a.push_back(b[i]);
			}
		}

		// If the packets are the same length, the XOR is already complete.
		// If this packet is longer, the rest of the result is zero XOR this = this.
		// Since this is the result, no copying is necessary.
	}

	inline void XorVectors(RVector& result, const RVector& a, const RVector& b)
	{
		// result will start as a copy of a or b, whichever is not shorter
		// than the other, since a shorter destination involves more work.
		if(a.size() > b.size()) {
			result = a;
			XorVectors(result, b);
		} else {
			result = b;
			XorVectors(result, a);
		}
	}

	inline bool VectorRangeAllArmored0(RVector::const_iterator& it, RVector::const_iterator& end)
	{
		while(it != end) {
			if(*it != ARMORED_0) {
				return false;
			}
		}
		return true;
	}

	inline bool VectorsEqual(const RVector& a, const RVector& b)
	{
		RVector::const_iterator ait = a.begin();
		RVector::const_iterator aend = a.end();
		RVector::const_iterator bit = b.begin();
		RVector::const_iterator bend = b.end();

		while((ait != aend) && (bit != bend)) {
			if(*ait != *bit) {
				return false;
			}
			++ait;
			++bit;
		}

		return
			(ait != aend) ? VectorRangeAllArmored0(ait, aend) :
			(bit != bend) ? VectorRangeAllArmored0(bit, bend) : true;
	}



	class ProcessEventCallbackDispatcher
	{
	private:
		const RVector& eventSextets;
		const RVector& valueSextets;
		const size_t bitCount;
		void * const context;
		ProcessEventCallback const callback;

		void CallCallback(size_t bitIndex, bool value)
		{
			callback(context, bitIndex, value);
		}

		inline void ProcessOneSextet(RChr eventSextet, RChr valueSextet, size_t bitStartIndex, size_t turns)
		{
			for(size_t subIndex = 0; subIndex < turns; ++subIndex) {
				RChr mask = 1 << subIndex;
				if(eventSextet & mask) {
					CallCallback(bitStartIndex + subIndex, (valueSextet & mask) != 0);
				}
			}
		}

		void ProcessAll()
		{
			size_t eventSextetCount = eventSextets.size();

			for(size_t sextetIndex = 0, bitStartIndex = 0; sextetIndex < eventSextetCount; ++sextetIndex, bitStartIndex += 6) {
				RChr eventSextet = eventSextets[sextetIndex];
				if(eventSextet == 0) {
					// No 1 bits this sextet.
					continue;
				}
				size_t turns = min(6, bitCount - bitStartIndex);

				if(turns > 0) {
					ProcessOneSextet(eventSextet, valueSextets[sextetIndex], bitStartIndex, turns);
				}

				if(turns < 6) {
					// bitCount has been reached.
					break;
				}
			}
		}


	public:
		ProcessEventCallbackDispatcher(
			const RVector& eventSextets,
			const RVector& valueSextets,
			const size_t bitCount,
			void * const context,
			ProcessEventCallback const callback
		) :
			eventSextets(eventSextets),
			valueSextets(valueSextets),
			bitCount(min(bitCount, eventSextets.size() * 6)),
			context(context),
			callback(callback)
		{
		}

		void RunEvents()
		{
			ProcessAll();
		}
	};

	inline void ProcessEventDataVectors(const RVector& eventSextets, const RVector& valueSextets, size_t bitCount, void * context, ProcessEventCallback callback)
	{
		ProcessEventCallbackDispatcher d(eventSextets, valueSextets, bitCount, context, callback);
		d.RunEvents();
	}
}

namespace SextetStream
{
	class Packet::Impl
	{
	private:
		RVector sextets;

		void SetToSextetDataLine(const RString& line, size_t left, size_t right)
		{
			size_t length = right - left;

			sextets.resize(length);

			RString::const_iterator stringIt = line.begin();
			std::advance(stringIt, left);

			RString::const_iterator stringEnd = stringIt;
			std::advance(stringEnd, length);

			RVector::iterator vectorIt = sextets.begin();

			for(; stringIt != stringEnd; ++stringIt, ++vectorIt) {
				*vectorIt = Armored(*stringIt);
			}
		}

	public:
		Impl()
		{
		}

		~Impl() {}

		void Clear()
		{
			sextets.clear();
		}

		void Copy(const Packet& packet)
		{
			sextets = packet._impl->sextets;
		}

		void SetToLine(const RString& line)
		{
			size_t left = FindCleanPacketLeft(line);
			size_t right = FindCleanPacketRight(line, left);
			SetToSextetDataLine(line, left, right);
		}

		void SetToXor(const Packet& b)
		{
			XorVectors(sextets, b._impl->sextets);
		}

		void SetToXor(const Packet& a, const Packet& b)
		{
			XorVectors(sextets, a._impl->sextets, b._impl->sextets);
		}

		void ProcessEventData(const Packet& eventData, size_t bitCount, void * context, ProcessEventCallback callback)
		{
			ProcessEventDataVectors(eventData._impl->sextets, sextets, bitCount, context, callback);
		}

		bool Equals(const Packet& b)
		{
			VectorsEqual(sextets, b._impl->sextets);
		}
	};

	Packet::Packet()
	{
		_impl = new Packet::Impl();
	}

	Packet::Packet(const Packet& packet)
	{
		_impl = new Packet::Impl();
		_impl->Copy(packet);
	}

	Packet::~Packet()
	{
		delete _impl;
	}

	void Packet::Clear()
	{
		_impl->Clear();
	}

	void Packet::Copy(const Packet& packet)
	{
		_impl->Copy(packet);
	}

	void Packet::SetToLine(const RString& line)
	{
		_impl->SetToLine(line);
	}

	void Packet::SetToXor(const Packet& b)
	{
		_impl->SetToXor(b);
	}

	void Packet::SetToXor(const Packet& a, const Packet& b)
	{
		_impl->SetToXor(a, b);
	}

	void Packet::ProcessEventData(const Packet& eventData, size_t bitCount, void * context, ProcessEventCallback callback)
	{
		_impl->ProcessEventData(eventData, bitCount, context, callback);
	}

	bool Packet::Equals(const Packet& b)
	{
		return _impl->Equals(b);
	}
}

/*
 * Copyright © 2016 Peter S. May
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
