
#include "global.h"
#include "arch/Sextets/Threads/MutexNames.h"
#include <sstream>

namespace Sextets
{
	namespace Threads
	{
		RString MutexNames::Make(const RString& ns, const RString& name, void * topic)
		{
			std::stringstream ss;
			ss << ns << "::" << name << "(" << topic << ")";
			return RString(ss.str());
		}

		RString MutexNames::Make(const RString& name, void * topic)
		{
			return Make("Sextets::Threads::MutexNames::Make", name, topic);
		}
	}
}

//KWH
