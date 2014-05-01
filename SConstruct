#SConstruct

# needed imports
import binascii
import subprocess
import sys
import os
import SCons.Util

def message(program,msg):
	print "%s: %s" % (program.program_message_prefix, msg)

# endianess-checker
def checkEndian():
    if ARGUMENTS.has_key('endian'):
        r = ARGUMENTS['endian']
        if r == "little" or r == "big":
            return r
        raise SCons.Errors.UserError("Unknown endian value: %s" % r)
    import struct
    array = struct.pack('cccc', '\x01', '\x02', '\x03', '\x04')
    i = struct.unpack('i', array)
    if i == struct.unpack('<i', array):
        return "little"
    elif i == struct.unpack('>i', array):
        return "big"
    return "unknown"

class ConfigureTests:
	class Collector:
		def __init__(self):
			self.tests = []
		def __call__(self,f):
			self.tests.append(f.__name__)
			return f
	class PreservedEnvironment:
		def __init__(self,env,keys):
			self.flags = {k: env.get(k, [])[:] for k in keys}
		def restore(self,env):
			env.Replace(**self.flags)
		def __getitem__(self,key):
			return self.flags.__getitem__(key)
	class ForceVerboseLog:
		def __init__(self,env):
			# Force verbose output to sconf.log
			self.cc_env_strings = {}
			for k in ['CXXCOMSTR']:
				try:
					self.cc_env_strings[k] = env[k]
					del env[k]
				except KeyError:
					pass
		def restore(self,env):
			# Restore potential quiet build options
			env.Replace(**self.cc_env_strings)
	_implicit_test = Collector()
	_custom_test = Collector()
	implicit_tests = _implicit_test.tests
	custom_tests = _custom_test.tests
	comment_not_supported = '/* not supported */'
	__flags_Werror = {k:['-Werror'] for k in ['CXXFLAGS']}
	__empty_main_program = 'int a();int a(){return 0;}'
	def __init__(self,msgprefix,user_settings):
		self.msgprefix = msgprefix
		self.user_settings = user_settings
		self.successful_flags = {}
		self.__repeated_tests = {}
		self.__automatic_compiler_tests = {
			'.cpp': self.check_cxx_works,
		}
	@classmethod
	def describe(cls,name):
		f = getattr(cls, name)
		if f.__doc__:
			lines = f.__doc__.rstrip().split('\n')
			if lines[-1].startswith("help:"):
				return lines[-1][5:]
		return None
	def _may_repeat(f):
		def wrap(self,*args,**kwargs):
			try:
				return self.__repeated_tests[f.__name__]
			except KeyError as e:
				pass
			r = f(self,*args,**kwargs)
			self.__repeated_tests[f.__name__] = r
			return r
		wrap.__name__ = 'repeat-wrap:' + f.__name__
		wrap.__doc__ = f.__doc__
		return wrap
	def _check_forced(self,context,name):
		return getattr(self.user_settings, 'sconf_%s' % name)
	def _check_macro(self,context,macro_name,macro_value,test,**kwargs):
		r = self.Compile(context, text="""
#define {macro_name} {macro_value}
{test}
""".format(macro_name=macro_name, macro_value=macro_value, test=test), **kwargs)
		if r:
			context.sconf.Define(macro_name, macro_value)
		else:
			context.sconf.Define(macro_name, self.comment_not_supported)
	def __compiler_test_already_done(self,context):
		pass
	def _check_compiler_works(self,context,ext):
		self.__automatic_compiler_tests.pop(ext, self.__compiler_test_already_done)(context)
	def _extend_successflags(self,k,v):
		self.successful_flags.setdefault(k, []).extend(v)
	def Compile(self,context,**kwargs):
		return self._Test(context,action=context.TryCompile, **kwargs)
	def Link(self,context,**kwargs):
		return self._Test(context,action=context.TryLink, **kwargs)
	def _Test(self,context,text,msg,action,ext='.cpp',testflags={},successflags={},skipped=None,successmsg=None,failuremsg=None,expect_failure=False):
		self._check_compiler_works(context,ext)
		context.Message('%s: checking %s...' % (self.msgprefix, msg))
		if skipped is not None:
			context.Result('(skipped){skipped}'.format(skipped=skipped))
			return
		env_flags = self.PreservedEnvironment(context.env, successflags.keys() + testflags.keys() + ['CPPDEFINES'])
		context.env.Append(**successflags)
		frame = None
		try:
			1//0
		except ZeroDivisionError:
			frame = sys.exc_info()[2].tb_frame.f_back
		while frame is not None:
			co_name = frame.f_code.co_name
			if co_name[0:6] == 'check_':
				forced = self._check_forced(context, co_name[6:])
				if forced is not None:
					if expect_failure:
						forced = not forced
					context.Result('(forced){inverted}{forced}'.format(forced='yes' if forced else 'no', inverted='(inverted)' if expect_failure else ''))
					return forced
				break
			frame = frame.f_back
		caller_modified_env_flags = self.PreservedEnvironment(context.env, self.__flags_Werror.keys() + testflags.keys())
		# Always pass -Werror
		context.env.Append(**self.__flags_Werror)
		context.env.Append(**testflags)
		cc_env_strings = self.ForceVerboseLog(context.env)
		undef_SDL_main = '#undef main	/* avoid -Dmain=SDL_main from libSDL */\n'
		r = action(undef_SDL_main + text + '\n', ext)
		if expect_failure:
			r = not r
		cc_env_strings.restore(context.env)
		context.Result((successmsg if r else failuremsg) or r)
		# On success, revert to base flags + successflags
		# On failure, revert to base flags
		if r:
			caller_modified_env_flags.restore(context.env)
			context.env.Replace(CPPDEFINES=env_flags['CPPDEFINES'])
			for d in successflags.pop('CPPDEFINES', []):
				if isinstance(d, str):
					d = (d,None)
				context.sconf.Define(d[0], d[1])
			for (k,v) in successflags.items():
				self._extend_successflags(k, v)
		else:
			env_flags.restore(context.env)
		return r
	def _check_system_library(self,context,header,main,lib,successflags={}):
		include = '\n'.join(['#include <%s>' % h for h in header])
		main_pre = '''
int main(int argc, char **argv){
'''
		main_post = 'return 0;}'
		text = include + main_pre + main + main_post
		# Test library.  On success, good.  On failure, test header to
		# give the user more help.
		if not successflags:
			successflags['LIBS'] = [lib]
		if self.Link(context, text=text, msg='for usable library ' + lib, successflags=successflags):
			return
		if self.Compile(context, text=text, msg='for usable header ' + header[-1]):
			raise SCons.Errors.StopError("Header %s is usable, but library %s is not usable." % (header[-1], lib))
		text = include + main_pre + main_post
		if self.Compile(context, text=text, msg='for parseable header ' + header[-1]):
			raise SCons.Errors.StopError("Header %s is parseable, but cannot compile the test program." % (header[-1]))
		raise SCons.Errors.StopError("Header %s is missing or unusable." % (header[-1]))
	@_custom_test
	def check_libphysfs(self,context):
		self._check_system_library(context,header=['physfs.h'],main='''
	PHYSFS_File *f;
	char b[1] = {0};
	PHYSFS_init("");
	f = PHYSFS_openWrite("a");
	PHYSFS_sint64 w = PHYSFS_write(f, b, 1, 1);
	(void)w;
	PHYSFS_close(f);
	f = PHYSFS_openRead("a");
	PHYSFS_sint64 r = PHYSFS_read(f, b, 1, 1);
	(void)r;
	PHYSFS_close(f);
''',
			lib='physfs'
		)
	@_custom_test
	def check_SDL_mixer(self,context):
		msg = 'whether to use SDL_mixer'
		context.Display('%s: checking %s...' % (self.msgprefix, msg))
		# SDL_mixer support?
		context.Result(self.user_settings.sdlmixer)
		if not self.user_settings.sdlmixer:
			return
		self._extend_successflags('CPPDEFINES', ['USE_SDLMIXER'])
		successflags = {}
		if self.user_settings.host_platform == 'darwin':
			successflags['FRAMEWORKS'] = ['SDL_mixer']
		self._check_system_library(context,header=['SDL_mixer.h'],main='''
	int i = Mix_Init(MIX_INIT_FLAC | MIX_INIT_OGG);
	(void)i;
	Mix_Pause(0);
	Mix_ResumeMusic();
	Mix_Quit();
''',
			lib='SDL_mixer', successflags=successflags)
	@_may_repeat
	@_implicit_test
	def check_cxx_works(self,context):
		"""
help:assume C++ compiler works
"""
		if not self.Compile(context, text=self.__empty_main_program, msg='whether C++ compiler works'):
			raise SCons.Errors.StopError("C++ compiler does not work.")
	@_custom_test
	def check_compiler_redundant_decl_warning(self,context):
		f = {'CXXFLAGS' : ['-Werror=redundant-decls']}
		if not self.Compile(context, text='int a();', msg='whether C++ compiler accepts -Werror=redundant-decls', testflags=f):
			return
		if not self.Compile(context, text='int a();int a();', msg='whether C++ compiler implements -Werror=redundant-decls', testflags=f, expect_failure=True):
			return
		# Test for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=15867
		text = '''
template <typename>
struct A {
	int a();
};
template <>
int A<int>::a();
'''
		self.Compile(context, text=text, msg='whether C++ compiler treats specializations as distinct', successflags=f)
	@_custom_test
	def check_attribute_error(self,context):
		"""
help:assume compiler supports __attribute__((error))
"""
		f = '''
void a()__attribute__((__error__("a called")));
void b();
void b(){
	%s
}
'''
		if self.Compile(context, text=f % '', msg='whether compiler accepts function __attribute__((__error__))') and \
			self.Compile(context, text=f % 'a();', msg='whether compiler understands function __attribute__((__error__))', expect_failure=True) and \
			self.Compile(context, text=f % 'if("0"[0]==\'1\')a();', msg='whether compiler optimizes function __attribute__((__error__))'):
			context.sconf.Define('DXX_HAVE_ATTRIBUTE_ERROR')
			context.sconf.Define('__attribute_error(M)', '__attribute__((__error__(M)))')
		else:
			context.sconf.Define('__attribute_error(M)', self.comment_not_supported)
	@_custom_test
	def check_builtin_constant_p(self,context):
		"""
help:assume compiler supports compile-time __builtin_constant_p
"""
		f = '''
int c(int);
static inline int a(int b){
	return __builtin_constant_p(b) ? 1 : %s;
}
int main(int argc, char **argv){
	return a(1);
}
'''
		if self.Compile(context, text=f % '2', msg='whether compiler accepts __builtin_constant_p') and \
			self.Link(context, text=f % 'c(b)', msg='whether compiler optimizes __builtin_constant_p'):
			context.sconf.Define('DXX_HAVE_BUILTIN_CONSTANT_P')
			context.sconf.Define('dxx_builtin_constant_p(A)', '__builtin_constant_p(A)')
		else:
			context.sconf.Define('dxx_builtin_constant_p(A)', '((void)(A),0)')
	@_custom_test
	def check_embedded_compound_statement(self,context):
		f = '''
int a();
int a(){
	return ({ 1 + 2; });
}
'''
		if self.Compile(context, text=f, msg='whether compiler understands embedded compound statements'):
			context.sconf.Define('DXX_HAVE_EMBEDDED_COMPOUND_STATEMENT')
	@_custom_test
	def check_attribute_alloc_size(self,context):
		"""
help:assume compiler supports __attribute__((alloc_size))
"""
		macro_name = '__attribute_alloc_size(A,...)'
		macro_value = '__attribute__((alloc_size(A, ## __VA_ARGS__)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
char*a(int)__attribute_alloc_size(1);
char*b(int,int)__attribute_alloc_size(1,2);
""", msg='for function __attribute__((alloc_size))')
	@_custom_test
	def check_attribute_format_arg(self,context):
		"""
help:assume compiler supports __attribute__((format_arg))
"""
		macro_name = '__attribute_format_arg(A)'
		macro_value = '__attribute__((format_arg(A)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
char*a(char*)__attribute_format_arg(1);
""", msg='for function __attribute__((format_arg))')
	@_custom_test
	def check_attribute_format_printf(self,context):
		"""
help:assume compiler supports __attribute__((format(printf)))
"""
		macro_name = '__attribute_format_printf(A,B)'
		macro_value = '__attribute__((format(printf,A,B)))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int a(char*,...)__attribute_format_printf(1,2);
int b(char*)__attribute_format_printf(1,0);
""", msg='for function __attribute__((format(printf)))')
	@_custom_test
	def check_attribute_malloc(self,context):
		"""
help:assume compiler supports __attribute__((malloc))
"""
		macro_name = '__attribute_malloc()'
		macro_value = '__attribute__((malloc))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int *a()__attribute_malloc();
""", msg='for function __attribute__((malloc))')
	@_custom_test
	def check_attribute_nonnull(self,context):
		"""
help:assume compiler supports __attribute__((nonnull))
"""
		macro_name = '__attribute_nonnull(...)'
		macro_value = '__attribute__((nonnull __VA_ARGS__))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
int a(int*)__attribute_nonnull();
int b(int*)__attribute_nonnull((1));
""", msg='for function __attribute__((nonnull))')
	@_custom_test
	def check_attribute_used(self,context):
		"""
help:assume compiler supports __attribute__((used))
"""
		macro_name = '__attribute_used'
		macro_value = '__attribute__((used))'
		self._check_macro(context,macro_name=macro_name,macro_value=macro_value,test="""
static void a()__attribute_used;
static void a(){}
""", msg='for function __attribute__((used))')
	@_may_repeat
	@_implicit_test
	def check_cxx11(self,context):
		"""
help:assume C++ compiler supports C++11
"""
		for f in ['-std=gnu++11', '-std=gnu++0x', '-std=c++11', '-std=c++0x']:
			r = self.Compile(context, text=self.__empty_main_program, msg='whether C++ compiler accepts {f}'.format(f=f), successflags={'CXXFLAGS': [f]})
			if r:
				return r
		return False
	def __cxx11(f):
		def wrap(self, context, *args, **kwargs):
			return f(self,context,cxx11_check_result=self.check_cxx11(context),*args,**kwargs)
		wrap.__name__ += ':' + f.__name__
		return wrap
	def __skip_missing_cxx11(self,cxx11_check_result):
		if cxx11_check_result:
			return None
		return 'no C++11 support'
	@_implicit_test
	def check_boost_array(self,context,f):
		"""
help:assume Boost.Array works
"""
		return self.Compile(context, text=f, msg='for Boost.Array', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_ARRAY']})
	@__cxx11
	@_implicit_test
	def check_cxx_array(self,context,f,cxx11_check_result):
		"""
help:assume <array> works
"""
		return self.Compile(context, text=f, msg='for <array>', skipped=self.__skip_missing_cxx11(cxx11_check_result), successflags={'CPPDEFINES' : ['DXX_HAVE_CXX_ARRAY']})
	@_implicit_test
	def check_cxx_tr1_array(self,context,f):
		"""
help:assume <tr1/array> works
"""
		return self.Compile(context, text=f, msg='for <tr1/array>', successflags={'CPPDEFINES' : ['DXX_HAVE_CXX_TR1_ARRAY']})
	@_custom_test
	def _check_cxx_array(self,context):
		f = '''
#include "compiler-array.h"
void a();void a(){array<int,2>b;b[0]=1;}
'''
		how = self.check_cxx_array(context, f) or self.check_boost_array(context, f) or self.check_cxx_tr1_array(context, f)
		if not how:
			raise SCons.Errors.StopError("C++ compiler does not support <array> or Boost.Array or <tr1/array>.")
	@__cxx11
	@_custom_test
	def check_cxx11_function_auto(self,context,cxx11_check_result):
		"""
help:assume compiler supports C++11 function declarator syntax
"""
		f = '''
auto f()->int;
'''
		if self.Compile(context, text=f, msg='for C++11 function declarator syntax', skipped=self.__skip_missing_cxx11(cxx11_check_result)):
			context.sconf.Define('DXX_HAVE_CXX11_FUNCTION_AUTO')
	def _check_static_assert_method(self,context,msg,f,testflags={},**kwargs):
		return self.Compile(context, text=f % 'true', msg=msg % 'true', testflags=testflags, **kwargs) and \
			self.Compile(context, text=f % 'false', msg=msg % 'false', expect_failure=True, successflags=testflags, **kwargs)
	@_implicit_test
	def check_boost_static_assert(self,context,f):
		"""
help:assume Boost.StaticAssert works
"""
		return self._check_static_assert_method(context, 'for Boost.StaticAssert when %s', f, testflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_STATIC_ASSERT']})
	@_implicit_test
	def check_c_typedef_static_assert(self,context,f):
		"""
help:assume C typedef-based static assertion works
"""
		return self._check_static_assert_method(context, 'for C typedef static assertion when %s', f, testflags={'CPPDEFINES' : ['DXX_HAVE_C_TYPEDEF_STATIC_ASSERT']})
	@__cxx11
	@_implicit_test
	def check_cxx11_static_assert(self,context,f,cxx11_check_result):
		"""
help:assume compiler supports C++ intrinsic static_assert
"""
		return self._check_static_assert_method(context, 'for C++11 intrinsic static_assert when %s', f, testflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_STATIC_ASSERT']}, skipped=self.__skip_missing_cxx11(cxx11_check_result))
	@_custom_test
	def _check_static_assert(self,context):
		f = '''
#include "compiler-static_assert.h"
static_assert(%s, "");
'''
		how = self.check_cxx11_static_assert(context,f) or self.check_boost_static_assert(context,f) or self.check_c_typedef_static_assert(context,f)
		if not how:
			raise SCons.Errors.StopError("C++ compiler does not support static_assert or Boost.StaticAssert or typedef-based static assertion.")
	@_implicit_test
	def check_boost_type_traits(self,context,f):
		"""
help:assume Boost.TypeTraits works
"""
		return self.Compile(context, text=f, msg='for Boost.TypeTraits', ext='.cpp', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_TYPE_TRAITS']})
	@__cxx11
	@_implicit_test
	def check_cxx11_type_traits(self,context,f,cxx11_check_result):
		"""
help:assume <type_traits> works
"""
		return self.Compile(context, text=f, msg='for <type_traits>', ext='.cpp', skipped=self.__skip_missing_cxx11(cxx11_check_result), successflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_TYPE_TRAITS']})
	@_custom_test
	def _check_type_traits(self,context):
		f = '''
#include "compiler-type_traits.h"
typedef tt::conditional<true,int,long>::type a;
typedef tt::conditional<false,int,long>::type b;
'''
		if self.check_cxx11_type_traits(context, f) or self.check_boost_type_traits(context, f):
			context.sconf.Define('DXX_HAVE_TYPE_TRAITS')
	@_implicit_test
	def check_boost_foreach(self,context,text):
		"""
help:assume Boost.Foreach works
"""
		return self.Compile(context, text=text, msg='for Boost.Foreach', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_FOREACH']})
	@__cxx11
	@_implicit_test
	def check_cxx11_range_for(self,context,text,cxx11_check_result):
		return self.Compile(context, text=text, msg='for C++11 range-based for', skipped=self.__skip_missing_cxx11(cxx11_check_result), successflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_RANGE_FOR']})
	@_custom_test
	def _check_range_based_for(self,context):
		f = '''
#include "compiler-range_for.h"
void a();
void a(){int b[2];range_for(int&c,b)c=0;}
'''
		if not self.check_cxx11_range_for(context, text=f) and not self.check_boost_foreach(context, text=f):
			raise SCons.Errors.StopError("C++ compiler does not support range-based for or Boost.Foreach.")
	@_implicit_test
	def check_pch(self,context):
		for how in [{'CXXFLAGS' : ['-x', 'c++-header']}]:
			result = self.Compile(context, text='', msg='whether compiler supports pre-compiled headers', testflags=how)
			if result:
				self.pch_flags = how
				return result
	@_custom_test
	def _check_pch(self,context):
		self.pch_flags = None
		msg = 'when to pre-compile headers'
		context.Display('%s: checking %s...' % (self.msgprefix, msg))
		if self.user_settings.pch:
			count = int(self.user_settings.pch)
		else:
			count = 0
		if count <= 0:
			context.Result('never')
			return
		context.Display('if used at least %u time%s\n' % (count, 's' if count > 1 else ''))
		if not self.check_pch(context):
			raise SCons.Errors.StopError("C++ compiler does not support pre-compiled headers.")
	@__cxx11
	@_custom_test
	def check_cxx11_explicit_bool(self,context,cxx11_check_result):
		"""
help:assume compiler supports explicit operator bool
"""
		f = '''
struct A{explicit operator bool();};
'''
		r = self.Compile(context, text=f, msg='for explicit operator bool', skipped=self.__skip_missing_cxx11(cxx11_check_result))
		macro_name = 'dxx_explicit_operator_bool'
		if r:
			context.sconf.Define(macro_name, 'explicit')
			context.sconf.Define('DXX_HAVE_EXPLICIT_OPERATOR_BOOL')
		else:
			context.sconf.Define(macro_name, self.comment_not_supported)
	@__cxx11
	@_custom_test
	def check_cxx11_explicit_delete(self,context,cxx11_check_result):
		"""
help:assume compiler supports explicitly deleted functions
"""
		f = '''
int a()=delete;
'''
		r = self.Compile(context, text=f, msg='for explicitly deleted functions', skipped=self.__skip_missing_cxx11(cxx11_check_result))
		macro_name = 'DXX_CXX11_EXPLICIT_DELETE'
		if r:
			context.sconf.Define(macro_name, '=delete')
			context.sconf.Define('DXX_HAVE_CXX11_EXPLICIT_DELETE')
		else:
			context.sconf.Define(macro_name, self.comment_not_supported)
	@__cxx11
	@_implicit_test
	def check_cxx11_free_begin(self,context,text,cxx11_check_result):
		return self.Compile(context, text=text, msg='for C++11 functions begin(), end()', skipped=self.__skip_missing_cxx11(cxx11_check_result), successflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_BEGIN']})
	@_implicit_test
	def check_boost_free_begin(self,context,text):
		return self.Compile(context, text=text, msg='for Boost.Range functions begin(), end()', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_BEGIN']})
	@_custom_test
	def _check_free_begin_function(self,context):
		f = '''
#include "compiler-begin.h"
struct A {
	typedef int *iterator;
	typedef const int *const_iterator;
	iterator begin(), end();
	const_iterator begin() const, end() const;
};
int b();
int b() {
	int a[1];
	A c;
	return begin(a) && end(a) && begin(c) && end(c);
}
int c();
int c() {
	const int a[1] = {0};
	const A b = {};
	return begin(a) && end(a) && begin(b) && end(b);
}
'''
		if not self.check_cxx11_free_begin(context, text=f) and not self.check_boost_free_begin(context, text=f):
			raise SCons.Errors.StopError("C++ compiler does not support free functions begin() and end().")
	@__cxx11
	@_implicit_test
	def check_cxx11_addressof(self,context,text,cxx11_check_result):
		return self.Compile(context, text=text, msg='for C++11 function addressof()', skipped=self.__skip_missing_cxx11(cxx11_check_result), successflags={'CPPDEFINES' : ['DXX_HAVE_CXX11_ADDRESSOF']})
	@_implicit_test
	def check_boost_addressof(self,context,text):
		return self.Compile(context, text=text, msg='for Boost.Utility function addressof()', successflags={'CPPDEFINES' : ['DXX_HAVE_BOOST_ADDRESSOF']})
	@_custom_test
	def _check_free_addressof_function(self,context):
		f = '''
#include "compiler-addressof.h"
struct A {
	void operator&();
};
A *a(A &b);
A *a(A &b) {
	return addressof(b);
}
'''
		if not self.check_cxx11_addressof(context, text=f) and not self.check_boost_addressof(context, text=f):
			raise SCons.Errors.StopError("C++ compiler does not support free function addressof().")
	@__cxx11
	@_implicit_test
	def check_cxx11_inherit_constructor(self,context,text,fmtargs,cxx11_check_result):
		"""
help:assume compiler supports inheriting constructors
"""
		macro_value = '''\\
	typedef B,##__VA_ARGS__ _dxx_constructor_base_type;\\
	using _dxx_constructor_base_type::_dxx_constructor_base_type;'''
		if self.Compile(context, text=text.format(macro_value=macro_value, **fmtargs), msg='for C++11 inherited constructors', skipped=self.__skip_missing_cxx11(cxx11_check_result)):
			return macro_value
		return None
	@__cxx11
	@_implicit_test
	def check_cxx11_variadic_forward_constructor(self,context,text,fmtargs,cxx11_check_result):
		"""
help:assume compiler supports variadic template-based constructor forwarding
"""
		macro_value = '''\\
    template <typename... Args>	\\
        D(Args&&... args) :	\\
            B,##__VA_ARGS__(std::forward<Args>(args)...) {}
'''
		if self.Compile(context, text='#include <algorithm>\n' + text.format(macro_value=macro_value, **fmtargs), msg='for C++11 variadic templates on constructors', skipped=self.__skip_missing_cxx11(cxx11_check_result)):
			return macro_value
		return None
	@_custom_test
	def _check_forward_constructor(self,context):
		text = '''
#define {macro_name}{macro_parameters} {macro_value}
struct A {{
	A(int);
}};
struct B:A {{
{macro_name}(B,A);
}};
'''
		macro_name = 'DXX_INHERIT_CONSTRUCTORS'
		macro_parameters = '(D,B,...)'
		# C++03 support is possible with enumerated out template
		# variations.  If someone finds a worthwhile compiler without
		# variadic templates, enumerated templates can be added.
		for f in [self.check_cxx11_inherit_constructor, self.check_cxx11_variadic_forward_constructor]:
			macro_value = f(context, text=text, fmtargs={'macro_name':macro_name, 'macro_parameters':macro_parameters})
			if macro_value:
				break
		if not macro_value:
			raise SCons.Errors.StopError("C++ compiler does not support constructor forwarding.")
		context.sconf.Define(macro_name + macro_parameters, macro_value)

class LazyObjectConstructor:
	def __lazy_objects(self,name,source):
		try:
			return self.__lazy_object_cache[name]
		except KeyError as e:
			def __strip_extension(self,name):
				return os.path.splitext(name)[0]
			value = []
			extra = {}
			for s in source:
				if isinstance(s, str):
					s = {'source': [s]}
				transform_target = s.get('transform_target', __strip_extension)
				for srcname in s['source']:
					t = transform_target(self, srcname)
					o = self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, t, self.env["OBJSUFFIX"]), source=srcname, **extra)
					if self.env._dxx_pch_node:
						self.env.Depends(o, self.env._dxx_pch_node)
					value.append(o)
			self.__lazy_object_cache[name] = value
			return value

	@staticmethod
	def create_lazy_object_getter(sources):
		name = repr(sources)
		return lambda s: s.__lazy_objects(name, sources)

	@classmethod
	def create_lazy_object_property(cls,sources):
		return property(cls.create_lazy_object_getter(sources))

	def __init__(self):
		self.__lazy_object_cache = {}

class FilterHelpText:
	def __init__(self):
		self.visible_arguments = []
	def FormatVariableHelpText(self, env, opt, help, default, actual, aliases):
		if not opt in self.visible_arguments:
			return ''
		l = []
		if default is not None:
			if isinstance(default, str) and not default.isalnum():
				default = '"%s"' % default
			l.append("default: {default}".format(default=default))
		actual = getattr(self, opt, None)
		if actual is not None:
			if isinstance(actual, str) and not actual.isalnum():
				actual = '"%s"' % actual
			l.append("current: {current}".format(current=actual))
		return "  {opt:13}  {help}".format(opt=opt, help=help) + (" [" + "; ".join(l) + "]" if l else '') + '\n'

class DXXCommon(LazyObjectConstructor):
	__shared_program_instance = [0]
	__endian = checkEndian()
	@property
	def program_message_prefix(self):
		return '%s.%d' % (self.PROGRAM_NAME, self.program_instance)
	# Settings which affect how the files are compiled
	class UserBuildSettings:
		# Paths for the Videocore libs/includes on the Raspberry Pi
		RPI_DEFAULT_VC_PATH='/opt/vc'
		default_OGLES_LIB = 'GLES_CM'
		_default_prefix = '/usr/local'
		def default_builddir(self):
			builddir_prefix = self.builddir_prefix
			builddir_suffix = self.builddir_suffix
			default_builddir = builddir_prefix or ''
			if builddir_prefix is not None or builddir_suffix is not None:
				fields = [
					self.host_platform,
					os.path.basename(self.CXX) if self.CXX else None,
				]
				compiler_flags = '\n'.join((getattr(self, attr) or '').strip() for attr in ['CPPFLAGS', 'CXXFLAGS'])
				if compiler_flags:
					# Mix in CRC of CXXFLAGS to get reasonable uniqueness
					# when flags are changed.  A full hash is
					# unnecessary here.
					crc = binascii.crc32(compiler_flags)
					if crc < 0:
						crc = crc + 0x100000000
					fields.append('{:08x}'.format(crc))
				if self.pch:
					fields.append('p%s' % self.pch)
				fields.append(''.join(a[1] if getattr(self, a[0]) else (a[2] if len(a) > 2 else '')
				for a in (
					('debug', 'dbg'),
					('profiler', 'prf'),
					('editor', 'ed'),
					('opengl', 'ogl', 'sdl'),
					('opengles', 'es'),
					('raspberrypi', 'rpi'),
				)))
				default_builddir += '-'.join([f for f in fields if f])
				if builddir_suffix is not None:
					default_builddir += builddir_prefix
			return default_builddir
		def default_memdebug(self):
			return self.debug
		# automatic setup for raspberrypi
		def default_opengles(self):
			if self.raspberrypi:
				return True
			return False
		def selected_OGLES_LIB(self):
			if self.raspberrypi:
				return 'GLESv2'
			return self.default_OGLES_LIB
		def __default_DATA_DIR(self):
			return self.prefix + '/share/games/' + self._program.target
		@staticmethod
		def _generic_variable(key,help,default):
			return (key, help, default)
		@staticmethod
		def _enum_variable(key,help,default,allowed_values):
			return EnumVariable(key, help, default, allowed_values)
		def _options(self):
			return (
			{
				'variable': BoolVariable,
				'arguments': [
					('sconf_%s' % name[6:], None, ConfigureTests.describe(name) or ('assume result of %s' % name)) for name in ConfigureTests.implicit_tests + ConfigureTests.custom_tests if name[0] != '_'
				],
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('raspberrypi', False, 'build for Raspberry Pi (automatically sets opengles and opengles_lib)'),
				),
			},
			{
				'variable': self._generic_variable,
				'arguments': (
					('rpi_vc_path', self.RPI_DEFAULT_VC_PATH, 'directory for RPi VideoCore libraries'),
					('opengles_lib', self.selected_OGLES_LIB, 'name of the OpenGL ES library to link against'),
					('prefix', self._default_prefix, 'installation prefix directory (Linux only)'),
					('sharepath', self.__default_DATA_DIR, 'directory for shared game data (Linux only)'),
					('pch', None, 'pre-compile headers used this many times'),
				),
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('debug', False, 'build DEBUG binary which includes asserts, debugging output, cheats and more output'),
					('memdebug', self.default_memdebug, 'build with malloc tracking'),
					('profiler', False, 'profiler build'),
					('opengl', True, 'build with OpenGL support'),
					('opengles', self.default_opengles, 'build with OpenGL ES support'),
					('asm', False, 'build with ASSEMBLER code (only with opengl=0, requires NASM and x86)'),
					('editor', False, 'include editor into build (!EXPERIMENTAL!)'),
					('sdlmixer', True, 'build with SDL_Mixer support for sound and music (includes external music support)'),
					('ipv6', False, 'enable IPv6 compability'),
					('use_udp', True, 'enable UDP support'),
					('use_tracker', True, 'enable Tracker support'),
					('verbosebuild', False, 'print out all compiler/linker messages during building'),
				),
			},
			{
				'variable': self._generic_variable,
				'arguments': (
					('CHOST', os.environ.get('CHOST'), 'CHOST of output'),
					('CXX', os.environ.get('CXX'), 'C++ compiler command'),
					('PKG_CONFIG', os.environ.get('PKG_CONFIG'), 'PKG_CONFIG to run (Linux only)'),
					('RC', os.environ.get('RC'), 'Windows resource compiler command'),
					('extra_version', None, 'text to append to version, such as VCS identity'),
				),
			},
			{
				'variable': self._generic_variable,
				'stack': ' ',
				'arguments': (
					('CPPFLAGS', os.environ.get('CPPFLAGS'), 'C preprocessor flags'),
					('CXXFLAGS', os.environ.get('CXXFLAGS'), 'C++ compiler flags'),
					('LDFLAGS', os.environ.get('LDFLAGS'), 'Linker flags'),
					('LIBS', os.environ.get('LIBS'), 'Libraries to link'),
				),
			},
			{
				'variable': self._enum_variable,
				'arguments': (
					('host_platform', 'linux' if sys.platform == 'linux2' else sys.platform, 'cross-compile to specified platform', {'allowed_values' : ['win32', 'darwin', 'linux']}),
				),
			},
			{
				'variable': self._generic_variable,
				'arguments': (
					('builddir_prefix', None, 'prefix to generated build directory'),
					('builddir_suffix', None, 'suffix to generated build directory'),
					# This must be last so that default_builddir will
					# have access to other properties.
					('builddir', self.default_builddir, 'build in specified directory'),
				),
			},
		)
		@staticmethod
		def _names(name,prefix):
			# Mask out the leading underscore form.
			return [('%s_%s' % (p, name)) for p in prefix if p]
		def __init__(self,program=None):
			self._program = program
		def register_variables(self,prefix,variables):
			self.known_variables = []
			for grp in self._options():
				variable = grp['variable']
				stack = grp.get('stack', None)
				for opt in grp['arguments']:
					(name,value,help) = opt[0:3]
					kwargs = opt[3] if len(opt) > 3 else {}
					names = self._names(name, prefix)
					for n in names:
						if n not in variables.keys():
							variables.Add(variable(key=n, help=help, default=None, **kwargs))
					if name not in variables.keys():
						filtered_help.visible_arguments.append(name)
						variables.Add(variable(key=name, help=help, default=None if callable(value) else value, **kwargs))
					self.known_variables.append((names + [name], value, stack))
					if stack:
						for n in names + [name]:
							variables.Add(self._generic_variable(key='%s_stop' % n, help=None, default=None))
		def read_variables(self,variables,d):
			for (namelist,dvalue,stack) in self.known_variables:
				value = None
				found_value = False
				for n in namelist:
					try:
						v = d[n]
						found_value = True
						if stack:
							if callable(v):
								value = v(dvalue=dvalue, value=value, stack=stack)
							else:
								if value:
									value = stack.join([value, v])
								else:
									value = v
							if d.get(n + '_stop', None):
								break
							continue
						value = v
						break
					except KeyError as e:
						pass
				if not found_value:
					value = dvalue
				if callable(value):
					value = value()
				setattr(self, namelist[-1], value)
			if self.builddir != '' and self.builddir[-1:] != '/':
				self.builddir += '/'
		def clone(self):
			clone = DXXCommon.UserBuildSettings(None)
			for grp in clone._options():
				for o in grp['arguments']:
					name = o[0]
					value = getattr(self, name)
					setattr(clone, name, value)
			return clone
	class UserInstallSettings:
		def _options(self):
			return (
			{
				'variable': self._generic_variable,
				'arguments': (
					('DESTDIR', None, 'installation stage directory'),
					('program_name', None, 'name of built program'),
				),
			},
			{
				'variable': BoolVariable,
				'arguments': (
					('register_install_target', True, 'report install target to SCons core'),
				),
			},
		)
	class UserSettings(UserBuildSettings,UserInstallSettings):
		def _options(self):
			return DXXCommon.UserBuildSettings._options(self) + DXXCommon.UserInstallSettings._options(self)
	# Base class for platform-specific settings processing
	class _PlatformSettings:
		tools = None
		ogllibs = ''
		osasmdef = None
		platform_sources = []
		platform_objects = []
		__pkg_config_sdl = {}
		def __init__(self,program,user_settings):
			self.__program = program
			self.user_settings = user_settings
		@property
		def env(self):
			return self.__program.env
		def find_sdl_config(self,program,env):
			if program.user_settings.PKG_CONFIG:
				pkgconfig = program.user_settings.PKG_CONFIG
			else:
				if program.user_settings.CHOST:
					pkgconfig = '%s-pkg-config' % program.user_settings.CHOST
				else:
					pkgconfig = 'pkg-config'
			cmd = '%s --cflags --libs sdl' % pkgconfig
			try:
				return self.__pkg_config_sdl[cmd]
			except KeyError as e:
				if (program.user_settings.verbosebuild != 0):
					message(program, "reading SDL settings from `%s`" % cmd)
				self.__pkg_config_sdl[cmd] = flags = env.ParseFlags('!' + cmd)
				return flags
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(_PlatformSettings):
		tools = ['mingw']
		osdef = '_WIN32'
		osasmdef = 'win32'
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL', 'WIN32_LEAN_AND_MEAN'])
	class DarwinPlatformSettings(_PlatformSettings):
		osdef = '__APPLE__'
		def __init__(self,program,user_settings):
			DXXCommon._PlatformSettings.__init__(self,program,user_settings)
			user_settings.asm = 0
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
			env.Append(CPPPATH = [os.path.join(os.getenv("HOME"), 'Library/Frameworks/SDL.framework/Headers'), '/Library/Frameworks/SDL.framework/Headers'])
			env.Append(FRAMEWORKS = ['ApplicationServices', 'Carbon', 'Cocoa', 'SDL'])
			if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
				env.Append(FRAMEWORKS = ['OpenGL'])
			env.Append(FRAMEWORKPATH = [os.path.join(os.getenv("HOME"), 'Library/Frameworks'), '/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks'])
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(_PlatformSettings):
		osdef = '__LINUX__'
		osasmdef = 'elf'
		__opengl_libs = ['GL', 'GLU']
		__pkg_config_sdl = {}
		def __init__(self,program,user_settings):
			DXXCommon._PlatformSettings.__init__(self,program,user_settings)
			if (user_settings.opengles == 1):
				self.ogllibs = [ user_settings.opengles_lib, 'EGL']
			else:
				self.ogllibs = self.__opengl_libs
		def adjust_environment(self,program,env):
			env.Append(CPPDEFINES = ['__LINUX__', 'HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL'])
			env.Append(CCFLAGS = ['-pthread'])

	def __init__(self):
		LazyObjectConstructor.__init__(self)
		self.sources = []
		self.__shared_program_instance[0] += 1
		self.program_instance = self.__shared_program_instance[0]

	@staticmethod
	def _collect_pch_candidates(target,source,env):
		for t in target:
			scanner = t.get_source_scanner(source[0])
			deps = scanner(source[0], env, scanner.path(env))
			for d in deps:
				ds = str(d)
				env.__dxx_pch_candidates[ds] = env.__dxx_pch_candidates.get(ds, 0) + 1
		return (target, source)

	@staticmethod
	def write_pch_inclusion_file(target, source, env):
		with open(str(target[0]), 'wt') as f:
			f.write('/* BEGIN PCH GENERATED FILE\n * Threshold=%u\n */\n' % env.__dxx_pch_inclusion_count)
			for (name,count) in env.__dxx_pch_candidates.items():
				if count >= env.__dxx_pch_inclusion_count:
					f.write('#include "%s"\t/* %u */\n' % (name, count))
					env.Depends(target, name)
			f.write('/* END PCH GENERATED FILE */\n')

	def create_pch_node(self,dirname,configure_pch_flags):
		if not configure_pch_flags:
			self.env._dxx_pch_node = None
			return
		dirname = os.path.join(self.user_settings.builddir, dirname)
		target = os.path.join(dirname, 'pch.h.gch')
		source = os.path.join(dirname, 'pch.cpp')
		self.env._dxx_pch_node = self.env.StaticObject(target=target, source=source, CXXFLAGS=self.env['CXXFLAGS'] + configure_pch_flags['CXXFLAGS'])
		self.env.Append(CXXFLAGS = ['-include', str(self.env._dxx_pch_node[0])[:-4], '-Winvalid-pch'])
		self.env.__dxx_pch_candidates = {}
		self.env.__dxx_pch_inclusion_count = int(self.user_settings.pch)
		self.env['BUILDERS']['StaticObject'].add_emitter('.cpp', self._collect_pch_candidates)
		self.env.Command(source, None, self.write_pch_inclusion_file)

	def _quote_cppdefine(self,s):
		r = ''
		prior = False
		for c in str(s):
			# No xdigit support in str
			if c in '/*-+= :._' or (c.isalnum() and not (prior and (c.isdigit() or c in 'abcdefABCDEF'))):
				r += c
				prior = False
			else:
				r += '\\\\x' + binascii.b2a_hex(c)
				prior = True
		return '\\"' + r + '\\"'

	def prepare_environment(self):
		# Prettier build messages......
		if (self.user_settings.verbosebuild == 0):
			builddir = self.user_settings.builddir if self.user_settings.builddir != '' else '.'
			self.env["CXXCOMSTR"]    = "Compiling %s %s $SOURCE" % (self.target, builddir)
			self.env["LINKCOMSTR"]   = "Linking %s $TARGET" % self.target
			self.env["ARCOMSTR"]     = "Archiving $TARGET ..."
			self.env["RANLIBCOMSTR"] = "Indexing $TARGET ..."

		# Use -Wundef to catch when a shared source file includes a
		# shared header that misuses conditional compilation.  Use
		# -Werror=undef to make this fatal.  Both are needed, since
		# gcc 4.5 silently ignores -Werror=undef.  On gcc 4.5, misuse
		# produces a warning.  On gcc 4.7, misuse produces an error.
		self.env.Append(CCFLAGS = ['-Wall', '-Wundef', '-Werror=missing-declarations', '-Werror=pointer-arith', '-Werror=undef', '-funsigned-char', '-Werror=implicit-int', '-Werror=implicit-function-declaration', '-Werror=format-security'])
		self.env.Append(CPPPATH = ['common/include', 'common/main', '.', self.user_settings.builddir])
		self.env.Append(CPPFLAGS = SCons.Util.CLVar('-Wno-sign-compare'))
		if (self.user_settings.editor == 1):
			self.env.Append(CPPPATH = ['common/include/editor'])
		# Get traditional compiler environment variables
		for cc in ['CXX', 'RC']:
			value = getattr(self.user_settings, cc)
			if value is not None:
				self.env[cc] = value
		for flags in ['CPPFLAGS', 'CXXFLAGS', 'LIBS']:
			value = getattr(self.user_settings, flags)
			if value is not None:
				self.env.Append(**{flags : SCons.Util.CLVar(value)})
		if self.user_settings.LDFLAGS:
			self.env.Append(LINKFLAGS = SCons.Util.CLVar(self.user_settings.LDFLAGS))

	def check_endian(self):
		# set endianess
		if (self.__endian == "big"):
			message(self, "BigEndian machine detected")
			self.asm = 0
			self.env.Append(CPPDEFINES = ['WORDS_BIGENDIAN'])
		elif (self.__endian == "little"):
			message(self, "LittleEndian machine detected")

	def check_platform(self):
		# windows or *nix?
		platform_name = self.user_settings.host_platform
		if self._argument_prefix_list:
			prefix = ' with prefix list %s' % list(self._argument_prefix_list)
		else:
			prefix = ''
		message(self, "compiling on %s for %s into %s%s" % (sys.platform, platform_name, self.user_settings.builddir or '.', prefix))
		if platform_name == 'win32':
			platform = self.Win32PlatformSettings
		elif platform_name == 'darwin':
			platform = self.DarwinPlatformSettings
		else:
			platform = self.LinuxPlatformSettings
		self.platform_settings = platform(self, self.user_settings)
		# Acquire environment object...
		self.env = Environment(ENV = os.environ, tools = platform.tools)
		# On Linux hosts, always run this.  It should work even when
		# cross-compiling a Rebirth to run elsewhere.
		if sys.platform == 'linux2':
			self.platform_settings.merge_sdl_config(self, self.env)
		self.platform_settings.adjust_environment(self, self.env)

	def process_user_settings(self):
		env = self.env
		# opengl or software renderer?
		if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
			if (self.user_settings.opengles == 1):
				message(self, "building with OpenGL ES")
				env.Append(CPPDEFINES = ['OGLES'])
			else:
				message(self, "building with OpenGL")
			env.Append(CPPDEFINES = ['OGL'])

		# assembler code?
		if (self.user_settings.asm == 1) and (self.user_settings.opengl == 0):
			message(self, "including: ASSEMBLER")
			env.Replace(AS = 'nasm')
			env.Append(ASCOM = ' -f ' + str(self.platform_settings.osasmdef) + ' -d' + str(self.platform_settings.osdef) + ' -Itexmap/ ')
			self.sources += asm_sources
		else:
			env.Append(CPPDEFINES = ['NO_ASM'])

		# debug?
		if (self.user_settings.debug == 1):
			message(self, "including: DEBUG")
			env.Prepend(CXXFLAGS = ['-g'])
		else:
			env.Append(CPPDEFINES = ['NDEBUG', 'RELEASE'])
			env.Prepend(CXXFLAGS = ['-O2'])
		if self.user_settings.memdebug:
			message(self, "including: MEMDEBUG")
			env.Append(CPPDEFINES = ['DEBUG_MEMORY_ALLOCATIONS'])

		# profiler?
		if (self.user_settings.profiler == 1):
			env.Append(CPPFLAGS = ['-pg'])

		#editor build?
		if (self.user_settings.editor == 1):
			env.Append(CPPDEFINES = ['EDITOR'])

		# IPv6 compability?
		if (self.user_settings.ipv6 == 1):
			env.Append(CPPDEFINES = ['IPv6'])

		# UDP support?
		if (self.user_settings.use_udp == 1):
			env.Append(CPPDEFINES = ['USE_UDP'])

		# Tracker support?
		if( self.user_settings.use_tracker == 1 ):
			env.Append( CPPDEFINES = [ 'USE_TRACKER' ] )
			env.Append( LIBS = [ 'curl' ] )

		# Raspberry Pi?
		if (self.user_settings.raspberrypi == 1):
			print "using Raspberry Pi vendor libs in %s" % self.user_settings.rpi_vc_path
			env.Append(CPPDEFINES = ['RPI', 'WORDS_NEED_ALIGNMENT'])
			env.Append(CPPPATH = [
				self.user_settings.rpi_vc_path+'/include',
				self.user_settings.rpi_vc_path+'/include/interface/vcos/pthreads',
				self.user_settings.rpi_vc_path+'/include/interface/vmcs_host/linux'])
			env.Append(LIBPATH = self.user_settings.rpi_vc_path + '/lib')
			env.Append(LIBS = ['bcm_host'])

class DXXArchive(DXXCommon):
	srcdir = 'common'
	target = 'dxx-common'
	__objects_common = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'2d/2dsline.cpp',
'2d/bitblt.cpp',
'2d/bitmap.cpp',
'2d/box.cpp',
'2d/canvas.cpp',
'2d/circle.cpp',
'2d/disc.cpp',
'2d/gpixel.cpp',
'2d/line.cpp',
'2d/pixel.cpp',
'2d/rect.cpp',
'2d/rle.cpp',
'2d/scalec.cpp',
'3d/clipper.cpp',
'3d/draw.cpp',
'3d/globvars.cpp',
'3d/instance.cpp',
'3d/matrix.cpp',
'3d/points.cpp',
'3d/rod.cpp',
'3d/setup.cpp',
'arch/sdl/joy.cpp',
'arch/sdl/rbaudio.cpp',
'arch/sdl/window.cpp',
'maths/fixc.cpp',
'maths/rand.cpp',
'maths/tables.cpp',
'maths/vecmat.cpp',
'misc/error.cpp',
'misc/hmp.cpp',
'misc/ignorecase.cpp',
'misc/strutil.cpp',
'misc/json/json_reader.cpp',
'misc/json/json_value.cpp',
'misc/json/json_writer.cpp',
'texmap/ntmap.cpp',
'texmap/scanline.cpp'
]
])
	objects_editor = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'editor/func.cpp',
'ui/button.cpp',
'ui/checkbox.cpp',
'ui/dialog.cpp',
'ui/file.cpp',
'ui/gadget.cpp',
'ui/icon.cpp',
'ui/inputbox.cpp',
'ui/keypad.cpp',
'ui/keypress.cpp',
'ui/keytrap.cpp',
'ui/listbox.cpp',
'ui/menu.cpp',
'ui/menubar.cpp',
'ui/message.cpp',
'ui/radio.cpp',
'ui/scroll.cpp',
'ui/ui.cpp',
'ui/uidraw.cpp',
'ui/userbox.cpp'
]
])
	# for non-ogl
	objects_arch_sdl = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'texmap/tmapflat.cpp'
]
])
	objects_arch_sdlmixer = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'arch/sdl/digi_mixer_music.cpp',
]
])
	class _PlatformSettings:
		def merge_sdl_config(self,program,env):
			flags = self.find_sdl_config(program, env)
			env.MergeFlags({k:flags[k] for k in flags.keys() if k[0] == 'C'})
	class Win32PlatformSettings(LazyObjectConstructor, DXXCommon.Win32PlatformSettings, _PlatformSettings):
		platform_objects = LazyObjectConstructor.create_lazy_object_property([
'common/arch/win32/messagebox.cpp'
])
		def __init__(self,program,user_settings):
			LazyObjectConstructor.__init__(self)
			DXXCommon.Win32PlatformSettings.__init__(self, program, user_settings)
			self.user_settings = user_settings
	class LinuxPlatformSettings(DXXCommon.LinuxPlatformSettings, _PlatformSettings):
		pass
	class DarwinPlatformSettings(DXXCommon.DarwinPlatformSettings, _PlatformSettings):
		pass
	@property
	def objects_common(self):
		objects_common = self.__objects_common
		return objects_common + self.platform_settings.platform_objects
	def __init__(self,user_settings):
		self.PROGRAM_NAME = 'DXX-Archive'
		self._argument_prefix_list = None
		DXXCommon.__init__(self)
		self.user_settings = user_settings.clone()
		self.check_platform()
		self.prepare_environment()
		self.check_endian()
		self.process_user_settings()
		self.configure_environment()
		self.create_pch_node(self.srcdir, self.configure_pch_flags)

	def configure_environment(self):
		fs = SCons.Node.FS.get_default_fs()
		builddir = fs.Dir(self.user_settings.builddir or '.')
		tests = ConfigureTests(self.program_message_prefix, self.user_settings)
		log_file=fs.File('sconf.log', builddir)
		conf = self.env.Configure(custom_tests = {
				k:getattr(tests, k) for k in tests.custom_tests
			},
			conf_dir=fs.Dir('.sconf_temp', builddir),
			log_file=log_file,
			config_h=fs.File('dxxsconf.h', builddir),
			clean=False,
			help=False
		)
		self.configure_added_environment_flags = tests.successful_flags
		self.configure_pch_flags = None
		if not conf.env:
			return
		try:
			for k in tests.custom_tests:
				getattr(conf, k)()
		except SCons.Errors.StopError as e:
			raise SCons.Errors.StopError(e.args[0] + '  See {log_file} for details.'.format(log_file=log_file), *e.args[1:])
		self.env = conf.Finish()
		self.configure_pch_flags = tests.pch_flags

class DXXProgram(DXXCommon):
	# version number
	VERSION_MAJOR = 0
	VERSION_MINOR = 58
	VERSION_MICRO = 1
	static_archive_construction = {}
	def _apply_target_name(self,name):
		return os.path.join(os.path.dirname(name), '.%s.%s' % (self.target, os.path.splitext(os.path.basename(name))[0]))
	objects_similar_arch_ogl = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/ogl/gr.cpp',
'arch/ogl/ogl.cpp',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_similar_arch_sdl = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/sdl/gr.cpp',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_similar_arch_sdlmixer = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'arch/sdl/digi_mixer.cpp',
'arch/sdl/jukebox.cpp'
]
],
		'transform_target':_apply_target_name,
	}])
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'2d/font.cpp',
'2d/palette.cpp',
'2d/pcx.cpp',
'3d/interp.cpp',
'arch/sdl/digi.cpp',
'arch/sdl/digi_audio.cpp',
'arch/sdl/event.cpp',
'arch/sdl/init.cpp',
'arch/sdl/key.cpp',
'arch/sdl/mouse.cpp',
'arch/sdl/timer.cpp',
'main/ai.cpp',
'main/aipath.cpp',
'main/automap.cpp',
'main/bm.cpp',
'main/cntrlcen.cpp',
'main/collide.cpp',
'main/config.cpp',
'main/console.cpp',
'main/controls.cpp',
'main/credits.cpp',
'main/digiobj.cpp',
'main/effects.cpp',
'main/endlevel.cpp',
'main/fireball.cpp',
'main/fuelcen.cpp',
'main/fvi.cpp',
'main/game.cpp',
'main/gamecntl.cpp',
'main/gamefont.cpp',
'main/gamemine.cpp',
'main/gamerend.cpp',
'main/gamesave.cpp',
'main/gameseg.cpp',
'main/gameseq.cpp',
'main/gauges.cpp',
'main/hostage.cpp',
'main/hud.cpp',
'main/iff.cpp',
'main/inferno.cpp',
'main/kconfig.cpp',
'main/kmatrix.cpp',
'main/laser.cpp',
'main/lighting.cpp',
'main/menu.cpp',
'main/mglobal.cpp',
'main/mission.cpp',
'main/morph.cpp',
'main/multi.cpp',
'main/multibot.cpp',
'main/newdemo.cpp',
'main/newmenu.cpp',
'main/object.cpp',
'main/paging.cpp',
'main/physics.cpp',
'main/piggy.cpp',
'main/player.cpp',
'main/playsave.cpp',
'main/polyobj.cpp',
'main/powerup.cpp',
'main/render.cpp',
'main/robot.cpp',
'main/scores.cpp',
'main/slew.cpp',
'main/songs.cpp',
'main/state.cpp',
'main/switch.cpp',
'main/terrain.cpp',
'main/texmerge.cpp',
'main/text.cpp',
'main/titles.cpp',
'main/vclip.cpp',
'main/wall.cpp',
'main/weapon.cpp',
'mem/mem.cpp',
'misc/args.cpp',
'misc/hash.cpp',
'misc/physfsx.cpp',
]
],
		'transform_target':_apply_target_name,
	}])
	objects_editor = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'editor/autosave.cpp',
'editor/centers.cpp',
'editor/curves.cpp',
'main/dumpmine.cpp',
'editor/eglobal.cpp',
'editor/elight.cpp',
'editor/eobject.cpp',
'editor/eswitch.cpp',
'editor/group.cpp',
'editor/info.cpp',
'editor/kbuild.cpp',
'editor/kcurve.cpp',
'editor/kfuncs.cpp',
'editor/kgame.cpp',
'editor/khelp.cpp',
'editor/kmine.cpp',
'editor/ksegmove.cpp',
'editor/ksegsel.cpp',
'editor/ksegsize.cpp',
'editor/ktmap.cpp',
'editor/kview.cpp',
'editor/med.cpp',
'editor/meddraw.cpp',
'editor/medmisc.cpp',
'editor/medrobot.cpp',
'editor/medsel.cpp',
'editor/medwall.cpp',
'editor/mine.cpp',
'editor/objpage.cpp',
'editor/segment.cpp',
'editor/seguvs.cpp',
'editor/texpage.cpp',
'editor/texture.cpp',
]
],
		'transform_target':_apply_target_name,
	}])

	objects_use_udp = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join('similar', f) for f in [
'main/net_udp.cpp',
]
],
		'transform_target':_apply_target_name,
	}])
	class UserSettings(DXXCommon.UserSettings):
		@property
		def BIN_DIR(self):
			# installation path
			return self.prefix + '/bin'
	class _PlatformSettings:
		def merge_sdl_config(self,program,env):
			flags = self.find_sdl_config(program, env)
			env.MergeFlags({k:flags[k] for k in flags.keys() if k[0] == 'C' or k[0] == 'L'})
	# Settings to apply to mingw32 builds
	class Win32PlatformSettings(DXXCommon.Win32PlatformSettings, _PlatformSettings):
		def __init__(self,program,user_settings):
			DXXCommon.Win32PlatformSettings.__init__(self,program,user_settings)
			user_settings.sharepath = ''
			self.platform_objects = self.platform_objects[:]
		def adjust_environment(self,program,env):
			DXXCommon.Win32PlatformSettings.adjust_environment(self, program, env)
			rcbasename = os.path.join(program.srcdir, 'arch/win32/%s' % program.target)
			self.platform_objects.append(env.RES(target='%s%s%s' % (program.user_settings.builddir, rcbasename, env["OBJSUFFIX"]), source='%s.rc' % rcbasename))
			env.Append(CPPPATH = [os.path.join(program.srcdir, 'arch/win32/include')])
			env.Append(LINKFLAGS = '-mwindows')
			env.Append(LIBS = ['glu32', 'wsock32', 'ws2_32', 'winmm', 'mingw32', 'SDLmain', 'SDL'])
	# Settings to apply to Apple builds
	class DarwinPlatformSettings(DXXCommon.DarwinPlatformSettings, _PlatformSettings):
		def __init__(self,program,user_settings):
			DXXCommon.DarwinPlatformSettings.__init__(self,program,user_settings)
			user_settings.sharepath = ''
		def adjust_environment(self,program,env):
			DXXCommon.DarwinPlatformSettings.adjust_environment(self, program, env)
			VERSION = str(program.VERSION_MAJOR) + '.' + str(program.VERSION_MINOR)
			if (program.VERSION_MICRO):
				VERSION += '.' + str(program.VERSION_MICRO)
			env['VERSION_NUM'] = VERSION
			env['VERSION_NAME'] = program.PROGRAM_NAME + ' v' + VERSION
			self.platform_sources = ['common/arch/cocoa/SDLMain.m', 'common/arch/carbon/messagebox.c']
	# Settings to apply to Linux builds
	class LinuxPlatformSettings(DXXCommon.LinuxPlatformSettings, _PlatformSettings):
		def __init__(self,program,user_settings):
			DXXCommon.LinuxPlatformSettings.__init__(self,program,user_settings)
			user_settings.sharepath += '/'

	@property
	def objects_common(self):
		objects_common = self.__objects_common
		if (self.user_settings.use_udp == 1):
			objects_common = objects_common + self.objects_use_udp
		return objects_common + self.platform_settings.platform_objects
	def __init__(self,prefix,variables):
		self.variables = variables
		self._argument_prefix_list = prefix
		DXXCommon.__init__(self)
		self.banner()
		self.user_settings = self.UserSettings(program=self)
		self.user_settings.register_variables(prefix=prefix, variables=self.variables)

	def init(self,substenv):
		self.user_settings.read_variables(self.variables, substenv)
		if not DXXProgram.static_archive_construction.has_key(self.user_settings.builddir):
			DXXProgram.static_archive_construction[self.user_settings.builddir] = DXXArchive(self.user_settings)
		self.check_platform()
		self.prepare_environment()
		self.check_endian()
		self.process_user_settings()
		self.register_program()

	def prepare_environment(self):
		DXXCommon.prepare_environment(self)
		archive = DXXProgram.static_archive_construction[self.user_settings.builddir]
		self.env.Append(**archive.configure_added_environment_flags)
		self.create_pch_node(self.srcdir, archive.configure_pch_flags)
		self.env.Append(CPPDEFINES = [('DXX_VERSION_MAJORi', str(self.VERSION_MAJOR)), ('DXX_VERSION_MINORi', str(self.VERSION_MINOR)), ('DXX_VERSION_MICROi', str(self.VERSION_MICRO))])
		# For PRIi64
		self.env.Append(CPPDEFINES = [('__STDC_FORMAT_MACROS',)])
		self.env.Append(CPPPATH = [os.path.join(self.srcdir, f) for f in ['include', 'main', 'arch/include']])

	def banner(self):
		VERSION_STRING = ' v' + str(self.VERSION_MAJOR) + '.' + str(self.VERSION_MINOR) + '.' + str(self.VERSION_MICRO)
		print '\n===== ' + self.PROGRAM_NAME + VERSION_STRING + ' =====\n'

	def check_platform(self):
		DXXCommon.check_platform(self)
		env = self.env
		# windows or *nix?
		if sys.platform == 'darwin':
			VERSION = str(self.VERSION_MAJOR) + '.' + str(self.VERSION_MINOR)
			if (self.VERSION_MICRO):
				VERSION += '.' + str(self.VERSION_MICRO)
			env['VERSION_NUM'] = VERSION
			env['VERSION_NAME'] = self.PROGRAM_NAME + ' v' + VERSION
		env.Append(LIBS = ['m'])

	def process_user_settings(self):
		DXXCommon.process_user_settings(self)
		env = self.env
		# opengl or software renderer?

		# profiler?
		if (self.user_settings.profiler == 1):
			env.Append(LINKFLAGS = '-pg')

		#editor build?
		if (self.user_settings.editor == 1):
			env.Append(CPPPATH = [os.path.join(self.srcdir, 'include/editor')])

		env.Append(CPPDEFINES = [('SHAREPATH', '\\"' + str(self.user_settings.sharepath) + '\\"')])

	def register_program(self):
		self._register_program(self.shortname)

	def _register_program(self,dxxstr,program_specific_objects=[]):
		env = self.env
		exe_target = os.path.join(self.srcdir, self.target)
		static_archive_construction = self.static_archive_construction[self.user_settings.builddir]
		objects = static_archive_construction.objects_common[:]
		objects.extend(self.objects_common)
		objects.extend(program_specific_objects)
		if (self.user_settings.sdlmixer == 1):
			objects.extend(static_archive_construction.objects_arch_sdlmixer)
			objects.extend(self.objects_similar_arch_sdlmixer)
		if (self.user_settings.opengl == 1) or (self.user_settings.opengles == 1):
			env.Append(LIBS = self.platform_settings.ogllibs)
			objects.extend(self.objects_similar_arch_ogl)
		else:
			message(self, "building with Software Renderer")
			objects.extend(static_archive_construction.objects_arch_sdl)
			objects.extend(self.objects_similar_arch_sdl)
		if (self.user_settings.editor == 1):
			objects.extend(self.objects_editor)
			objects.extend(static_archive_construction.objects_editor)
			exe_target += '-editor'
		if self.user_settings.program_name:
			exe_target = self.user_settings.program_name
		versid_cppdefines=self.env['CPPDEFINES'][:]
		versid_build_environ = []
		for k in ['CXX', 'CPPFLAGS', 'CXXFLAGS', 'LINKFLAGS']:
			versid_cppdefines.append(('DESCENT_%s' % k, self._quote_cppdefine(self.env[k])))
			versid_build_environ.append('RECORD_BUILD_VARIABLE(%s);' % k)
		a = self.env['CXX'].split(' ')
		v = subprocess.Popen(a + ['--version'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		(so,se) = v.communicate(None)
		if not v.returncode and (so or se):
			v = (so or se).split('\n')[0]
			versid_cppdefines.append(('DESCENT_%s' % 'CXX_version', self._quote_cppdefine(v)))
			versid_build_environ.append('RECORD_BUILD_VARIABLE(%s);' % 'CXX_version')
		versid_cppdefines.append(('RECORD_BUILD_ENVIRONMENT', "'" + ''.join(versid_build_environ) + "'"))
		if self.user_settings.extra_version:
			versid_cppdefines.append(('DESCENT_VERSION_EXTRA', self._quote_cppdefine(self.user_settings.extra_version)))
		versid_objlist = [self.env.StaticObject(target='%s%s%s' % (self.user_settings.builddir, self._apply_target_name(s), self.env["OBJSUFFIX"]), source=s, CPPDEFINES=versid_cppdefines) for s in ['similar/main/vers_id.cpp']]
		if self.env._dxx_pch_node:
			self.env.Depends(versid_objlist[0], self.env._dxx_pch_node)
		objects.extend(versid_objlist)
		# finally building program...
		exe_node = env.Program(target=os.path.join(self.user_settings.builddir, str(exe_target)), source = self.sources + objects)
		if self.user_settings.host_platform != 'darwin':
			if self.user_settings.register_install_target:
				install_dir = (self.user_settings.DESTDIR or '') + self.user_settings.BIN_DIR
				env.Install(install_dir, exe_node)
				env.Alias('install', install_dir)
		else:
			syspath = sys.path[:]
			cocoa = 'common/arch/cocoa'
			sys.path += [cocoa]
			import tool_bundle
			sys.path = syspath
			tool_bundle.TOOL_BUNDLE(env)
			env.MakeBundle(os.path.join(self.user_settings.builddir, self.PROGRAM_NAME + '.app'), exe_node,
					'free.%s-rebirth' % dxxstr, os.path.join(self.srcdir, '%sgl-Info.plist' % dxxstr),
					typecode='APPL', creator='DCNT',
					icon_file=os.path.join(cocoa, '%s-rebirth.icns' % dxxstr),
					subst_dict={'%sgl' % dxxstr : exe_target},	# This is required; manually update version for Xcode compatibility
					resources=[[s, s] for s in [os.path.join(self.srcdir, 'English.lproj/InfoPlist.strings')]])

	def GenerateHelpText(self):
		return self.variables.GenerateHelpText(self.env)

class D1XProgram(DXXProgram):
	PROGRAM_NAME = 'D1X-Rebirth'
	target = 'd1x-rebirth'
	srcdir = 'd1x-rebirth'
	shortname = 'd1x'
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPDEFINES = [('DXX_BUILD_DESCENT_I', 1)])

	# general source files
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'main/bmread.cpp',
'main/custom.cpp',
'main/snddecom.cpp',
]
],
	}])
	@property
	def objects_common(self):
		return self.__objects_common + DXXProgram.objects_common.fget(self)

	# for editor
	__objects_editor = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'main/hostage.cpp',
'editor/ehostage.cpp',
]
],
	}])
	@property
	def objects_editor(self):
		return self.__objects_editor + DXXProgram.objects_editor.fget(self)

	# assembler related
	objects_asm = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'texmap/tmap_ll.asm',
'texmap/tmap_flt.asm',
'texmap/tmapfade.asm',
'texmap/tmap_lin.asm',
'texmap/tmap_per.asm'
]
])

class D2XProgram(DXXProgram):
	PROGRAM_NAME = 'D2X-Rebirth'
	target = 'd2x-rebirth'
	srcdir = 'd2x-rebirth'
	shortname = 'd2x'
	def prepare_environment(self):
		DXXProgram.prepare_environment(self)
		# Flags and stuff for all platforms...
		self.env.Append(CPPDEFINES = [('DXX_BUILD_DESCENT_II', 1)])

	# general source files
	__objects_common = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'libmve/decoder8.cpp',
'libmve/decoder16.cpp',
'libmve/mve_audio.cpp',
'libmve/mvelib.cpp',
'libmve/mveplay.cpp',
'main/escort.cpp',
'main/gamepal.cpp',
'main/movie.cpp',
'main/segment.cpp',
'misc/physfsrwops.cpp',
]
],
	}])
	@property
	def objects_common(self):
		return self.__objects_common + DXXProgram.objects_common.fget(self)

	# for editor
	__objects_editor = DXXCommon.create_lazy_object_property([{
		'source':[os.path.join(srcdir, f) for f in [
'main/bmread.cpp',
]
],
	}])
	@property
	def objects_editor(self):
		return self.__objects_editor + DXXProgram.objects_editor.fget(self)

	# assembler related
	objects_asm = DXXCommon.create_lazy_object_property([os.path.join(srcdir, f) for f in [
'texmap/tmap_ll.asm',
'texmap/tmap_flt.asm',
'texmap/tmapfade.asm',
'texmap/tmap_lin.asm',
'texmap/tmap_per.asm'
]
])

variables = Variables(['site-local.py'], ARGUMENTS)
filtered_help = FilterHelpText()
variables.FormatVariableHelpText = filtered_help.FormatVariableHelpText
def register_program(program):
	s = program.shortname
	import itertools
	l = [v for (k,v) in ARGLIST if k == s or k == 'dxx'] or [1]
	# Fallback case: build the regular configuration.
	if len(l) == 1:
		try:
			if int(l[0]):
				return [program((s,''), variables)]
			return []
		except ValueError:
			# If not an integer, treat this as a configuration profile.
			pass
	r = []
	seen = set()
	for e in l:
		for prefix in itertools.product(*[v.split('+') for v in e.split(',')]):
			if prefix in seen:
				continue
			seen.add(prefix)
			prefix = ['%s%s%s' % (s, '_' if p else '', p) for p in prefix] + list(prefix)
			r.append(program(prefix, variables))
	return r
d1x = register_program(D1XProgram)
d2x = register_program(D2XProgram)

# show some help when running scons -h
h = 'DXX-Rebirth, SConstruct file help:' + """

	Type 'scons' to build the binary.
	Type 'scons install' to build (if it hasn't been done) and install.
	Type 'scons -c' to clean up.
	
	Extra options (add them to command line, like 'scons extraoption=value'):
	d1x=[0/1]        Disable/enable D1X-Rebirth
	d1x=prefix-list  Enable D1X-Rebirth with prefix-list modifiers
	d2x=[0/1]        Disable/enable D2X-Rebirth
	d2x=prefix-list  Enable D2X-Rebirth with prefix-list modifiers
"""
substenv = SCons.Environment.SubstitutionEnvironment()
variables.Update(substenv)
for d in d1x + d2x:
	d.init(substenv)
	h += d.PROGRAM_NAME + ('.%d:\n' % d.program_instance) + d.GenerateHelpText()
Help(h)

#EOF
