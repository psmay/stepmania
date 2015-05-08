
#ifndef Sextets_Threads_MutexNames_h
#define Sextets_Threads_MutexNames_h

namespace Sextets
{
	namespace Threads
	{
		/** @brief Utilities for generating distinguished (though not
		 * guaranteed unique) name strings for mutexes. */
		class MutexNames
		{
			private:
				virtual void _prevent_instances() = 0;

			public:
				/** @brief Generates a name for a mutex based on two strings
				 * and the address of a topic object. */
				static RString Make(const RString& ns, const RString& name, void * topic);
				/** @brief Generates a name for a mutex based on a string
				 * and the address of a topic object. */
				static RString Make(const RString& name, void * topic);
		};
	}
}

#endif // defined Sextets_Threads_MutexNames_h
//KWH
