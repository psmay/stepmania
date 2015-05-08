
#ifndef Sextets_Threads_MutexNames_h
#define Sextets_Threads_MutexNames_h

namespace Sextets
{
	namespace Threads
	{
		class MutexNames
		{
			private:
				virtual void _prevent_instances() = 0;

			public:
				static RString Make(const RString& ns, const RString& name, void * topic);
				static RString Make(const RString& name, void * topic);
		};
	}
}

#endif // defined Sextets_Threads_MutexNames_h
