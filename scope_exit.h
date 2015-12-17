#pragma once

namespace cxxdetail
{
	template <typename FuncType>
	class SCOPEEXIT
	{		
	public:
		SCOPEEXIT(const FuncType _func) :func(_func){}
		~SCOPEEXIT(){ if (!dismissed){ func(); } }
	private:
		FuncType func;
		bool dismissed = false;
	};
	template <typename F>
	SCOPEEXIT<F> MakeScopeExit(F f) {
		return SCOPEEXIT<F>(f);
	};
}

#define DO_STRING_JOIN(arg1, arg2) arg1 ## arg2
#define STRING_JOIN(arg1, arg2) DO_STRING_JOIN(arg1, arg2)
#define SCOPEEXITEXEC(code) auto STRING_JOIN(scope_exit_object_, __LINE__) = cxxdetail::MakeScopeExit([&](){code;});