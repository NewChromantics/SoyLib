#include "SoyJava.h"
#include <sstream>
#include <thread>
#include <SoyDebug.h>
#include <fcntl.h>



namespace Java
{
	template<typename TYPE>
	TYPE	GetFieldCall(TJniObject& Object,const std::string& FieldName,jfieldID Field);
	
	template<typename TYPE>
	TYPE	GetField(TJniObject& Object,const std::string& FieldName);
}



std::ostream& operator<<(std::ostream &out,const TJniObject& in)
{
	out << in.GetClassName() << "(" << (int)(in.GetWeakObject()) << ")";
	return out;
}



inline std::string	GetSignature_ObjectReturn(const TJniClass& ReturnType);
inline std::string	GetSignature_ObjectReturn(const TJniClass& ReturnType,const std::string& SigTypea);


JNIEnv& java()
{
	return Java::GetContext();
}

//	throw c++ exception if we have a java exception pending
//	gr: maybe make this a specific std::exception?
//	todo: make recursive-safe
void ThrowJavaException(const std::string& Context,bool ThrowRegardless=false)
{
	//	gr: current unexplained hacks
	static bool ReleaseThrowable = false;
	static bool ReleaseThrowableClass = false;
	
	//	grab exception so we can try and extract a description
	TJniLocalObject<jthrowable> Exceptionj( java().ExceptionOccurred(), ReleaseThrowable );
	if ( !Exceptionj )
	{
		//	no java exception
		if ( ThrowRegardless )
			throw Soy::AssertException( std::string("Forced java exception: ") + Context );

		return;
	}

	//	can't fetch extended exception info until we've cleared the exception
	//	ExceptionOccurred() doesn't pop the exception it seems
	java().ExceptionDescribe();
	java().ExceptionClear();
	
	std::stringstream ExceptionError;
	ExceptionError << "Java exception in " << Context << ": ";
	

	
	static bool DebugExceptionHandling = false;
	
	//	gr: maybe change to an atomic or an actual[non-recursive] lock?
	static bool RecursionLock = false;
	if ( !RecursionLock )
	{
		RecursionLock = true;
		//	try and get class and description from it
		try
		{
			//	get class of the exception, and turn it into an object

			if ( DebugExceptionHandling )	std::Debug << "JavaException: Getting object class" << std::endl;
			TJniLocalObject<jclass> ExceptionClassJ( java().GetObjectClass(Exceptionj.mObject), ReleaseThrowableClass );
			auto Exceptionjo = static_cast<jobject>( Exceptionj.mObject );
			if ( DebugExceptionHandling )	std::Debug << "JavaException: setting up Exception object " << "(" << Exceptionjo << "," << ExceptionClassJ << ")" << std::endl;
			TJniObject Exception( Exceptionjo, ExceptionClassJ.mObject, nullptr );
			
			//	get exception class name
			if ( DebugExceptionHandling )	std::Debug << "JavaException: setting up ClassClass object" << std::endl;
			TJniClass ClassClass("java/lang/Class");
			TJniObject ExceptionClass( static_cast<jobject>(ExceptionClassJ.mObject), ClassClass.GetWeakClass(), ClassClass.GetClassName().c_str() );
			
			if ( DebugExceptionHandling )	std::Debug << "JavaException: get exception class name" << std::endl;
			std::string NameString = ExceptionClass.CallStringMethod("getName");
			ExceptionError << NameString << ": ";
			
			//	get description
			if ( DebugExceptionHandling )	std::Debug << "JavaException: get exception message name" << std::endl;
			std::string MessageString = Exception.CallStringMethod("getMessage");
			ExceptionError << MessageString;
		}
		catch (...)
		{
			ExceptionError << "(Error getting exception description.)";
		}
		RecursionLock = false;
	}
	else
	{
		ExceptionError << "(Recursion getting java exception)";
	}

	throw Soy::AssertException( ExceptionError.str() );
}

void Java::IsOkay(const std::string& Context,bool ThrowRegardless)
{
	ThrowJavaException( Context, ThrowRegardless );
}

void ThrowJavaException(std::ostream& Context,bool ThrowRegardless)
{
	auto ContextStr = Soy::StreamToString( Context );
	//ThrowJavaException( Soy::StreamToString( Context ), ThrowRegardless );
	ThrowJavaException( ContextStr, ThrowRegardless );
}

//	catch a specific exception class. if a different exception occurs, throw. If no exception, return false
bool CatchJavaException(const std::string& ExceptionClass,const std::string& Context)
{
	//	todo
	ThrowJavaException( Context, true );
	return false;
}


template<typename TYPE>
TYPE Java::GetFieldCall(TJniObject& Object,const std::string& FieldName,jfieldID Field)
{
	std::stringstream Error;
	Error << __func__ << " type not implemented";
	throw Soy::AssertException( Error.str() );
};

template<>
int Java::GetFieldCall<int>(TJniObject& Object,const std::string& FieldName,jfieldID Field)
{
	auto Result = java().GetIntField( Object.GetWeakObject(), Field );
	return Result;
};

template<>
long Java::GetFieldCall<long>(TJniObject& Object,const std::string& FieldName,jfieldID Field)
{
	auto Result = java().GetLongField( Object.GetWeakObject(), Field );
	return Result;
};


template<typename TYPE>
TYPE Java::GetField(TJniObject& Object,const std::string& FieldName)
{
	auto Signature = GetSignatureType<TYPE>();
	auto FieldId = java().GetFieldID( Object.GetWeakClass(), FieldName.c_str(), Signature.c_str() );
	if ( !FieldId )
		ThrowJavaException(__func__);
	return GetFieldCall<TYPE>( Object, FieldName, FieldId );
};



inline std::string	GetSignature_ObjectReturn(const TJniClass& ReturnType)
{
	std::stringstream ss;
	ss << "()" << GetSignatureType(ReturnType);
	return ss.str();
}

inline std::string	GetSignature_ObjectReturn(const TJniClass& ReturnType,const std::string& SigTypea)
{
	std::stringstream ss;
	ss << "(" << SigTypea << ")" << GetSignatureType(ReturnType);
	return ss.str();
}


inline std::string	GetSignature_ObjectReturn(const TJniClass& ReturnType,const std::string& SigTypea,const std::string& SigTypeb,const std::string& SigTypec)
{
	std::stringstream ss;
	ss << "(" << SigTypea << SigTypeb << SigTypec << ")" << GetSignatureType(ReturnType);
	return ss.str();
}





TJniString::TJniString(const std::string& String) :
	mString		( nullptr ),
	mCache		( String )
{
	mString = java().NewStringUTF( String.c_str() );
	//std::Debug << "Alloc JString: " << mCache << std::endl;
	Java::IsOkay( std::string("Creating JString ")+String );
}

TJniString::~TJniString()
{
	if ( mString )
	{
		//std::Debug << "Release JString: " << mCache << std::endl;
		java().DeleteLocalRef( mString );
		mString = nullptr;
	}
}

std::string TJniString::Get() const
{
	return Soy::JStringToString( mString );
}


std::string Soy::JStringToString(jstring Stringj)
{
	if ( !Stringj )
		return "<null jstring>";
	
	//	get utf string
	char const* Stringc = java().GetStringUTFChars( Stringj, 0 );
	ThrowJavaException("java().GetStringUTFChars");

	//	copy & release
	std::string String( Stringc ? Stringc : "nullutf" );
	java().ReleaseStringUTFChars( Stringj, Stringc );
	ThrowJavaException("java().ReleaseStringUTFChars");
	
	return String;
}


std::string Soy::JStringToString(TJniLocalObject<jstring>& Stringj)
{
	return JStringToString( Stringj.mObject );
}


class TBufferMeta
{
public:
	TBufferMeta(TJniObject& Buffer) :
		mOffset	( 0 )
	{
		//	todo: check type of Buffer is java.nio.bytebuffer
		
		//	gr: arrayOffset() throws and we can catch it, but produces spurious exception output, so check first
		if ( Buffer.CallBoolMethod("hasArray") )
		{
			try
			{
				//	throws if !hasArray (array backed in OS)
				mOffset = Buffer.CallIntMethod("arrayOffset");
			}
			catch(...)
			{
			}
		}
		mLimit = Buffer.CallIntMethod("limit");
		mCapacity = Buffer.CallIntMethod("capacity");
		mPosition = Buffer.CallIntMethod("position");
	}
	
	size_t	GetSize(int EnforcedSize)
	{
		//Soy::Assert( BufferOffset+BufferSize<=BufferDataMaxSize, "Explicit buffer size is larger than capacity" );
		//Soy::Assert( BufferSize >= 0, "Invalid buffer size");
		if ( EnforcedSize >= 0 )
			return EnforcedSize;
		
		//BufferDataMaxSize - BufferOffset;
		
		return mLimit - mPosition;
	}
	
	int		mOffset;
	int		mCapacity;
	int		mLimit;
	int		mPosition;
};

std::ostream& operator<<(std::ostream &out,const TBufferMeta& in)
{
	out << "TBufferMeta(limit=" << in.mLimit << ",mCapacity=" << in.mCapacity << ",mOffset=" << in.mOffset << ",mposition=" << in.mPosition << ")";
	return out;
}


FixedRemoteArray<uint8> Java::GetBufferArray(TJniObject& Buffer,int LimitSize)
{
	TBufferMeta Meta(Buffer);
	size_t Position = Meta.mPosition;
	
	/*
	 //https://github.com/xnio/xnio-native/blob/master/libxnio/src/write.c
	 jint pos = (*env)->GetIntField(env, bufObj, Buffer_pos);
	 jint lim = (*env)->GetIntField(env, bufObj, Buffer_lim);
	 
	 void *buffer = (*env)->GetDirectBufferAddress(env, bufObj);
	 if (buffer) {
	 iov[idx].iov_base = buffer + pos;
	 iov[idx].iov_len = lim - pos;
	 */
	
	
	//	gr: this may be on a different [JAVA] thread than where we allocated, or access it from later. This may need to be sync'd...
	//	access byte buffer directly
	JNIEnv& Java = Java::GetContext();
	auto* BufferData = static_cast<uint8*>( Java.GetDirectBufferAddress( Buffer.GetWeakObject() ) );
	auto BufferDataMaxSize = Java.GetDirectBufferCapacity( Buffer.GetWeakObject() );
	Soy::Assert( BufferDataMaxSize != -1, "Java VM doesn not support direct buffer access");
	BufferDataMaxSize = Meta.mLimit;

	//	don't report this
	if ( BufferData == nullptr )
		throw Soy::AssertException("Could not access java buffer directly.");

	//	gr: do we apply the offset?
	//size_t Offset = Meta.mOffset;
	size_t Offset = 0;
	//	gr: apply the position for practical use
	Offset += Position;

	size_t Length = BufferDataMaxSize-Offset;
	if ( LimitSize >= 0 )
	{
		std::stringstream Error;
		Error << "Trying to restrict ByteBuffer from " << Length << " to " << LimitSize;
		Soy::Assert( LimitSize <= Length, Error.str() );
		Length = LimitSize;
	}
	
	return FixedRemoteArray<uint8>( BufferData+Offset, Length );
}

void Java::ArrayToBuffer(const ArrayBridge<uint8>&& Data,TJniObject& Buffer,const std::string& Context,int BufferSize)
{
	std::stringstream TimerName;
	TimerName << __func__ << " " << Context << " ArraySize=" << Data.GetSize();
	Soy::TScopeTimerPrint Timer( TimerName.str().c_str(), 5 );

	//	fast method if possible
	//	gr: fast method doesn't seem to have an effect. need to bake to davlik heap?
	/*
	try
	{
		//	copy data into array
		auto BufferArray = GetBufferArray( Buffer );
		BufferArray.Copy( Data );
		
		//	update "end of buffer"
		int NewPosition = Meta.mPosition + Data.GetSize();
		Buffer.CallObjectMethod("position", "java.nio.Buffer", NewPosition );

		std::Debug << "Post fast-buffer copy..." << std::endl;
		TBufferMeta NewMeta(Buffer);
		return;
	}
	catch (...)
	{
	}
	*/
	
	//	buffered with byte[] array
	try
	{
		TimerName << " <<<< new byte[] SetByteArrayRegion PATH";
		Timer.SetName( TimerName.str() );

		//std::Debug << "pre intermediate-buffer copy..." << TBufferMeta(Buffer) << std::endl;

		TJniLocalObject<jbyteArray> ByteArrayj( java().NewByteArray( Data.GetSize() ) );
		java().SetByteArrayRegion( ByteArrayj.mObject, 0, Data.GetDataSize(), (jbyte*)Data.GetArray() );
		
		/*
		jboolean IsCopy=false;
		auto* ByteArrayjBytes = java().GetByteArrayElements( ByteArrayj.mObject, &IsCopy );
		FixedRemoteArray<uint8> ByteArray( reinterpret_cast<uint8*>(ByteArrayjBytes), Data.GetSize() );
		
		//	copy & bake elements
		bool CopyChangesBackToJava = true;
		ByteArray.Copy( Data );
		java().ReleaseByteArrayElements( ByteArrayj.mObject, ByteArrayjBytes, CopyChangesBackToJava ? 0 : JNI_ABORT );
*/
		
		TJniClass ReturnClass("java.nio.ByteBuffer");
		std::string Signature("([BII)Ljava/nio/ByteBuffer;");
		
		std::string MethodName("put");
		int Start = 0;
		int Length = Data.GetSize();
		auto Method = Buffer.GetMethod( MethodName, Signature );
		TJniLocalObject<jobject> ResultObjectj = java().CallObjectMethod( Buffer.GetWeakObject(), Method, ByteArrayj.mObject, Start, Length );
		ThrowJavaException( std::string(__func__) + MethodName );
		//std::Debug << "Post intermediate-buffer copy..." << TBufferMeta(Buffer) << std::endl;
		return;
	}
	catch(std::exception& e)
	{
		std::Debug << __func__ << " intermediate buffer copy failed; " << e.what() << std::endl;
	}
	
	
	TimerName << " <<<< SLOW PATH";
	Timer.SetName( TimerName.str() );

	for ( int i=0;	i<Data.GetSize();	i++ )
	{
		Buffer.CallObjectMethod("put", "java.nio.ByteBuffer", Data[i] );
	}
}

void Java::BufferToArray(TJniObject& Buffer,ArrayBridge<uint8>&& Data,const std::string& Context,int BufferSize)
{
	TBufferMeta Meta(Buffer);

	std::stringstream TimerName;
	TimerName << __func__ << " " << Context << " BufferSize=" << BufferSize << "/" << Meta.GetSize(-1);
	Soy::TScopeTimerPrint Timer( TimerName.str().c_str(), 5 );

	
	//	try fast version
	try
	{
		auto BufferArray = GetBufferArray( Buffer, BufferSize );
		//	gr: if this is the wrong way around we get a segfault... catch this!
		TimerName << " <<<< FAST PATH";
		Timer.SetName( TimerName.str() );
		Data.Copy( BufferArray );
		return;
	}
	catch (...)
	{
	}
	
	TimerName << " <<<< SLOW PATH";
	Timer.SetName( TimerName.str() );
	
	Array<uint8> RawBuffer;
	auto Size = Meta.GetSize(BufferSize);
	RawBuffer.SetSize( Size );
	for ( int i=0;	i<Size;	i++ )
	{
		//	index doesn't apply to java access
		//int ByteIndex = i + Meta.GetFirstIndex();
		int ByteIndex = i;
		auto Byte = Buffer.CallByteMethod("get",ByteIndex);
		RawBuffer[i] = Byte;
	}
	
	Data.Copy(RawBuffer);
}



void TJniClass::NormaliseClassName(std::string& ClassName)
{
	//ClassName = java().NormalizeJniClassDescriptor( ClassName.c_str() );
	// Rewrite '.' as '/' for backwards compatibility.
	std::replace( ClassName.begin(), ClassName.end(), '.', '/' );
}


TJniClass::TJniClass(const char* ClassName) :
	mClass				( nullptr ),
	mClassName			( ClassName?ClassName:"<null classname>" ),
	mAutoReleaseClass	( false )
{
	NormaliseClassName(mClassName);
	
	//	class needs a global ref... everywhere does a c-style cast to jclass...
	//	http://stackoverflow.com/questions/14765776/jni-error-app-bug-accessed-stale-local-reference-0xbc00021-index-8-in-a-tabl
	TJniLocalObject<jclass> Class( java().FindClass( mClassName.c_str() ) );
	ThrowJavaException( std::string("FindClass ") + GetClassName() );
	mClass = reinterpret_cast<jclass>( java().NewGlobalRef( Class.mObject ) );
	if ( !mClass )
	{
		ThrowJavaException( std::string("Failed to get global ref for class ") + GetClassName(), true );
		return;
	}
	mAutoReleaseClass = true;
}

TJniClass::TJniClass(const jclass& Class,const char* ClassName) :
	mClass				( nullptr ),
	mClassName			( ClassName ? ClassName : "<native class>" ),
	mAutoReleaseClass	( false )
{
	NormaliseClassName(mClassName);
	
	mClass = reinterpret_cast<jclass>( java().NewGlobalRef( Class ) );
	ThrowJavaException( __func__ );
	mAutoReleaseClass = true;

	//	extract class name
	if ( ClassName == nullptr )
	{
		//	gr: this is recursive because of ExceptionClass
		/*
		TJniClass ClassClass("java/lang/Class");
		TJniObject ExceptionClass( static_cast<jobject>(mClass), ClassClass.GetWeakClass(), true );
		try
		{
			mClassName = ExceptionClass.CallStringMethod("getName");
		}
		catch ( std::exception& e )
		{
			std::Debug << "Exception getting class name: " << e.what() << std::endl;
			mClassName = "<exception getting class name>";
		}
		 */
	}
}

TJniClass::TJniClass(const TJniClass& Class) :
	mClass				( nullptr ),
	mClassName			( "<unread classname>" ),
	mAutoReleaseClass	( false )
{
	*this = Class;
}

TJniClass& TJniClass::operator=(const TJniClass& that)
{
	/*
	): Revision: '11'
	D/CrashAnrDetector( 3634): ABI: 'arm'
	D/CrashAnrDetector( 3634): pid: 22144, tid: 26485, name: Thread-27514  >>> com.NewChromantics.PopMovieTextureDemo <<<
	D/CrashAnrDetector( 3634): signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x0
	D/CrashAnrDetector( 3634):     r0 00000000  r1 f499fbf8  r2 00000001  r3 d9e4b088
	D/CrashAnrDetector( 3634):     r4 d22c8c6c  r5 d9cf4058  r6 db33d470  r7 00000000
	D/CrashAnrDetector( 3634):     r8 ee522f40  r9 df463488  sl f70e44c9  fp d22c8db0
	D/CrashAnrDetector( 3634):     ip db365310  sp d22c87f8  lr dad889cb  pc dad39d5c  cpsr 000f0030
	D/CrashAnrDetector( 3634):     d0  322d646165726837  d1  6261742066657235
	D/CrashAnrDetector( 3634):     d2  6a2074736e6f6331  d3  0000677500006734
	D/CrashAnrDetector( 3634):     d4  0000000000006775  d5  0000000000000000
	D/CrashAnrDetector( 3634):     d6  0001000100010001  d7  0000000000000004
	D/CrashAnrDetector( 3634):     d8  0000000000000000  d9  0000000000000000
	D/CrashAnrDetector( 3634):     d10 0000000000000000  d11 0000000000000000
	D/CrashAnrDetector( 3634):     d12 0000000000000000  d13 0000000000000000
	D/CrashAnrDetector( 3634):     d14 0000000000000000  d15 0000000000000000
	D/CrashAnrDetector( 3634):     d16 ee492c00ee492c00  d17 00000000002450d4
	D/CrashAnrDetector( 3634):     d18 00000000002499df  d19 0001000100010001
	D/CrashAnrDetector( 3634):     d20 0001000100010001  d21 0000000000000000
	D/CrashAnrDetector( 3634):     d22 0000000000000000  d23 ffffffffffffffff
	D/CrashAnrDetector( 3634):     d24 0080008000800080  d25 0080008000800080
	D/CrashAnrDetector( 3634):     d26 0000000000000000  d27 0001000100010001
	D/CrashAnrDetector( 3634):     d28 0800080008000800  d29 0800080008000800
	D/CrashAnrDetector( 3634):     d30 0000000000000000  d31 ffffffffffffffff
	D/CrashAnrDetector( 3634):     scr 2000001b
	D/CrashAnrDetector( 3634):
	D/CrashAnrDetector( 3634): backtrace:
	D/CrashAnrDetector( 3634):     #00 pc 0006ed5c  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so (TJniClass::operator=(TJniClass const&)+27)
	D/CrashAnrDetector( 3634):     #01 pc 0006ed19  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so (TJniClass::TJniClass(TJniClass const&)+36)
	D/CrashAnrDetector( 3634):     #02 pc 00070079  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so (TJniObject::TJniObject(TJniObject const&)+8)
	D/CrashAnrDetector( 3634):     #03 pc 0005b6eb  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so (AndroidMediaEncoder::Alloc(std::shared_ptr<TSurfaceTexture>, Platform::TMediaFormat&)+18)
	D/CrashAnrDetector( 3634):     #04 pc 0005d48f  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so
	D/CrashAnrDetector( 3634):     #05 pc 000a0edd  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so (SoyEvent<bool>::OnTriggered(bool&)+132)
	D/CrashAnrDetector( 3634):     #06 pc 000a0c53  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so (SoyWorker::Loop()+42)
	D/CrashAnrDetector( 3634):     #07 pc 000a16ef  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so
	D/CrashAnrDetector( 3634):     #08 pc 000e4153  /data/app/com.NewChromantics.PopMovieTextureDemo-1/lib/arm/libPopMovieTexture.so
*/
	
	
	if ( this != &that )
	{
		//std::Debug << "TJniClass(" << this << ")=TJniClass(" << (&that) << ")" << std::endl;
		//std::Debug << "TJniClass=TJniClass(" << GetClassName() << " <- "  << that.GetClassName() << ")" << std::endl;
		
		Release();
		
		//	increment global reference if the other was a strong reference
		if ( that.mAutoReleaseClass )
		{
			mClass = reinterpret_cast<jclass>( java().NewGlobalRef( that.mClass ) );
			if ( mClass == nullptr )
				ThrowJavaException( std::stringstream() << GetClassName() << "::NewGlobalRef() failed in class copy constructor", true );
			mAutoReleaseClass = true;
			this->mClassName = that.mClassName;
		}
		else
		{
			this->mClass = that.mClass;
			this->mAutoReleaseClass = that.mAutoReleaseClass;
		}
	}
	return *this;
}


void TJniClass::Release()
{
	if ( mClass )
	{
		if ( mAutoReleaseClass )
		{
			java().DeleteGlobalRef( mClass );
			ThrowJavaException(__func__);
		}
		mClass = nullptr;
	}
}


TJniObject TJniClass::CallStaticObjectMethod(const std::string& MethodName,const std::string& ReturnClassName,const std::string& ParamA)
{
	TJniClass ReturnClass( ReturnClassName.c_str() );
	auto Signature = GetSignature_ObjectReturn( ReturnClass, GetSignatureType<std::string>() );
	auto Method = GetStaticMethod( MethodName, Signature );
	
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	auto ResultObjectj = java().CallStaticObjectMethod( GetWeakClass(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
	
	//	construct returning object
	TJniObject ResultObject( ResultObjectj, ReturnClass.GetWeakClass(), ReturnClass.GetClassName().c_str() );
	return ResultObject;
}


TJniObject TJniClass::CallStaticObjectMethod(const std::string& MethodName,const std::string& ReturnClassName,const std::string& ParamA,const int& ParamB,const int& ParamC)
{
	TJniClass ReturnClass( ReturnClassName.c_str() );
	auto Signature = GetSignature_ObjectReturn( ReturnClass, GetSignatureType<std::string>(), GetSignatureType<int>(), GetSignatureType<int>() );
	auto Method = GetStaticMethod( MethodName, Signature );
	
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	int ParamBj = ParamB;
	int ParamCj = ParamC;
	auto ResultObjectj = java().CallStaticObjectMethod( GetWeakClass(), Method, ParamAj, ParamBj, ParamCj );
	ThrowJavaException( std::string(__func__) + MethodName );
	
	//	construct returning object
	TJniObject ResultObject( ResultObjectj, ReturnClass.GetWeakClass(), ReturnClass.GetClassName().c_str() );
	return ResultObject;
}


TJniObject TJniClass::CallStaticObjectMethod(const std::string& MethodName,const std::string& ReturnClassName,int ParamA)
{
	TJniClass ReturnClass( ReturnClassName.c_str() );
	auto Signature = GetSignature_ObjectReturn( ReturnClass, GetSignatureType<int>() );
	auto Method = GetStaticMethod( MethodName, Signature );
	
	auto ResultObjectj = java().CallStaticObjectMethod( GetWeakClass(), Method, ParamA );
	ThrowJavaException( std::string(__func__) + MethodName );
	
	//	construct returning object
	TJniObject ResultObject( ResultObjectj, ReturnClass.GetWeakClass(), ReturnClass.GetClassName().c_str() );
	return ResultObject;
}



TJniObject::TJniObject(jobject Object,jclass Class,const char* ClassName) :
	TJniClass			( Class, ClassName ),
	mObject				( nullptr ),
	mAutoReleaseObject	( false )
{
	auto AllocFunc = [&Object]
	{
		return Object;
	};
	Alloc( AllocFunc );

}

TJniObject::TJniObject(const char* ClassName) :
	TJniClass			( ClassName ),
	mObject				( nullptr ),
	mAutoReleaseObject	( false )
{
	auto ConstructorSignature = GetSignature<void>();
	auto Constructor = GetMethod( "<init>", ConstructorSignature );
	if ( !Constructor )
	{
		ThrowJavaException( std::stringstream() << GetClassName() << "::constructor(" << ConstructorSignature << ") not found", true );
		return;
	}
	
	auto ConstructorCallback = [this,&Constructor]()
	{
		return java().NewObject( mClass, Constructor );
	};
	
	Alloc( ConstructorCallback );
}

TJniObject::TJniObject(const char* ClassName,const int& Value) :
	TJniClass		( ClassName ),
	mObject			( nullptr ),
	mAutoReleaseObject	( false )
{
	auto ConstructorSignature = GetSignature<void>(GetSignatureType<int>());
	auto Constructor = GetMethod( "<init>", ConstructorSignature );
	if ( !Constructor )
	{
		ThrowJavaException( std::stringstream() << GetClassName() << "::constructor(" << ConstructorSignature << ") not found", true );
		return;
	}
	
	auto ConstructorCallback = [this,&Value,&Constructor]()
	{
		int Integer = Value;
		return java().NewObject( mClass, Constructor, Integer );
	};
	
	Alloc( ConstructorCallback );
}


TJniObject::TJniObject(const char* ClassName,const int& ParamA,const bool& ParamB) :
	TJniClass		( ClassName ),
	mObject			( nullptr ),
	mAutoReleaseObject	( false )
{
	auto ConstructorSignature = GetSignature<void>(GetSignatureType<int>(),GetSignatureType<bool>());
	auto Constructor = GetMethod( "<init>", ConstructorSignature );
	if ( !Constructor )
	{
		ThrowJavaException( std::stringstream() << GetClassName() << "::constructor(" << ConstructorSignature << ") not found", true );
		return;
	}
	
	auto ConstructorCallback = [this,&ParamA,&ParamB,&Constructor]()
	{
		int Integer = ParamA;
		bool Boolean = ParamB;
		return java().NewObject( mClass, Constructor, Integer, Boolean );
	};
	
	Alloc( ConstructorCallback );
}


TJniObject::TJniObject(const char* ClassName,TJniObject& Value) :
	TJniClass		( ClassName ),
	mObject			( nullptr ),
	mAutoReleaseObject	( false )
{
	auto ConstructorSignature = GetSignature<void>( GetSignatureType(Value) );
	auto Constructor = GetMethod( "<init>", ConstructorSignature );
	if ( !Constructor )
	{
		ThrowJavaException( std::stringstream() << GetClassName() << "::constructor(" << ConstructorSignature << ") not found", true );
		return;
	}

	auto ConstructorCallback = [this,&Value,&Constructor]()
	{
		return java().NewObject( mClass, Constructor, Value.GetStrongObject() );
	};
	
	Alloc( ConstructorCallback );
}


TJniObject::TJniObject(const char* ClassName,const std::string& ParamA) :
TJniClass		( ClassName ),
mObject			( nullptr ),
mAutoReleaseObject	( false )
{
	auto ConstructorSignature = GetSignature<void>(GetSignatureType<std::string>());
	auto Constructor = GetMethod( "<init>", ConstructorSignature );
	if ( !Constructor )
	{
		ThrowJavaException( std::stringstream() << GetClassName() << "::constructor(" << ConstructorSignature << ") not found", true );
		return;
	}
	
	auto ConstructorCallback = [this,&ParamA,&Constructor]()
	{
		TJniString ParamAj( ParamA );
		return java().NewObject( mClass, Constructor, ParamAj.mString );
	};
	
	Alloc( ConstructorCallback );
}


TJniObject::TJniObject(const char* ClassName,const std::string& ParamA,const std::string& ParamB) :
	TJniClass			( ClassName ),
	mObject				( nullptr ),
	mAutoReleaseObject	( false )
{
	auto ConstructorSignature = GetSignature<void>(GetSignatureType<std::string>(),GetSignatureType<std::string>());
	auto Constructor = GetMethod( "<init>", ConstructorSignature );
	if ( !Constructor )
	{
		ThrowJavaException( std::stringstream() << GetClassName() << "::constructor(" << ConstructorSignature << ") not found", true );
		return;
	}
	
	auto ConstructorCallback = [this,&ParamA,&ParamB,&Constructor]()
	{
		TJniString ParamAj( ParamA );
		TJniString ParamBj( ParamB );
		return java().NewObject( mClass, Constructor, ParamAj.mString, ParamBj.mString );
	};
	
	Alloc( ConstructorCallback );
}


TJniObject::TJniObject(const TJniObject& that) :
	TJniClass			( that ),
	mObject				( nullptr ),
	mAutoReleaseObject	( false )
{
	*this = that;
}

TJniObject& TJniObject::operator=(const TJniObject& that)
{
	if ( this != &that )
	{
		Release();

		//	do class copy
		auto* thisclass = static_cast<TJniClass*>(this);
		auto& thatclass = static_cast<const TJniClass&>(that);
		*thisclass = thatclass;

		//	increment global reference if the other was a strong reference
		if ( that.mAutoReleaseObject )
		{
			mObject = java().NewGlobalRef( that.mObject );
			if ( mObject == nullptr )
				ThrowJavaException( std::stringstream() << GetClassName() << "::NewGlobalRef() failed in object copy constructor", true );
			mAutoReleaseObject = true;
		}
		else
		{
			this->mObject = that.mObject;
			this->mAutoReleaseObject = that.mAutoReleaseObject;
		}
	}
	return *this;
}

void TJniObject::PreFunctionCall(const char* Func,const std::string& MethodName,...)
{
	
}


int TJniObject::GetIntField(const std::string& Name)
{
	return Java::GetField<int>( *this, Name );
}

int TJniObject::GetLongField(const std::string& Name)
{
	return Java::GetField<long>( *this, Name );
}

void TJniObject::Alloc(std::function<jobject()> Constructor)
{
	Soy::Assert( Constructor != nullptr, "Constructor expected" );
	
	//	call provided constructing
	TJniLocalObject<jobject> LocalObject( Constructor() );
	if ( !LocalObject )
		ThrowJavaException( std::stringstream() << GetClassName() << "::NewObject() failed", true );
	
	mObject = java().NewGlobalRef( LocalObject.mObject );
	if ( !mObject )
		ThrowJavaException( std::stringstream() << GetClassName() << "::NewGlobalRef() failed", true );
	mAutoReleaseObject = true;
}


void TJniObject::Release()
{
	TJniClass::Release();
	
	if ( mObject != nullptr )
	{
		if ( mAutoReleaseObject )
		{
			java().DeleteGlobalRef( mObject );
			ThrowJavaException( std::string(__func__) + " DeleteGlobalRef" );
		}
		mObject = nullptr;
	}
}

jobject TJniObject::GetStrongObject()
{
	if ( !mObject )
		return nullptr;
	
	auto GlobalRef = java().NewGlobalRef( mObject );
	ThrowJavaException( std::string(__func__) + " NewGlobalRef" );
	return GlobalRef;
}



void TJniObject::SetCallback(const std::string& SetListenerMethodName,std::function<void(void)> Callback)
{
	//	setup the listener object
	//	create listener
	//void setOnVideoSizeChangedListener (MediaPlayer.OnVideoSizeChangedListener listener)
}



jmethodID TJniClass::GetMethod(const std::string& MethodName,const std::string& Signature)
{
	std::stringstream Error;
	Error << "cannot " << __func__ << "( " << MethodName << ", " << Signature << ") with null class. (" << mClassName << ") OS JNI abort app ";
	Soy::Assert( mClass != nullptr, Error.str() );
	
	jmethodID Method = java().GetMethodID( mClass, MethodName.c_str(), Signature.c_str() );
	
	if ( !Method )
		ThrowJavaException( std::stringstream() << "Failed to find method: " << GetClassName() << " :: " << MethodName << " [" << Signature << "]", true );
	
	return Method;
}

jmethodID TJniClass::GetStaticMethod(const std::string& MethodName,const std::string& Signature)
{
	std::stringstream Error;
	Error << "cannot " << __func__ << "( " << MethodName << ", " << Signature << ") with null class. (" << mClassName << ") OS JNI abort app ";
	Soy::Assert( mClass != nullptr, Error.str() );
	
	jmethodID Method = java().GetStaticMethodID( mClass, MethodName.c_str(), Signature.c_str() );
	
	if ( !Method )
		ThrowJavaException( std::stringstream() << "Failed to find static method: " << GetClassName() << " :: " << MethodName << " [" << Signature << "]", true );
	
	return Method;
}

void TJniObject::CallVoidMethod(const std::string& MethodName)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<void>( MethodName );
	java().CallVoidMethod( GetWeakObject(), Method );
	ThrowJavaException( std::string(__func__) + MethodName );
}

void TJniObject::CallVoidMethod(const std::string& MethodName,const int& ParamA)
{
	PreFunctionCall(__func__,MethodName,ParamA);
	auto Method = GetMethod<void>( MethodName, GetSignatureType<int>() );
	int ParamAj = ParamA;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
}

void TJniObject::CallVoidMethod(const std::string& MethodName,const long& ParamA)
{
	PreFunctionCall(__func__,MethodName,ParamA);
	//std::Debug << __func__ << "(" << MethodName << "," << ParamA << ") " << std::endl;
	auto Method = GetMethod<void>( MethodName, GetSignatureType<long>() );
	jlong ParamAj = ParamA;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
}


void TJniObject::CallVoidMethod(const std::string& MethodName,const std::string& ParamA)
{
	PreFunctionCall(__func__,MethodName);
	//std::Debug << __func__ << "(" << MethodName << "," << ParamA << ") " << std::endl;
	auto Method = GetMethod<void>( MethodName, GetSignatureType<std::string>() );
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
}


void TJniObject::CallVoidMethod(const std::string& MethodName,const std::string& ParamA,const int& ParamB)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<void>( MethodName, GetSignatureType<std::string>(), GetSignatureType<int>() );
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	auto ParamBj = ParamB;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj, ParamBj );
	ThrowJavaException( std::string(__func__) + MethodName );
}


void TJniObject::CallVoidMethod(const std::string& MethodName,const std::string& ParamA,const long& ParamB)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<void>( MethodName, GetSignatureType<std::string>(), GetSignatureType<long>() );
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	jlong ParamBj = ParamB;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj, ParamBj );
	ThrowJavaException( std::string(__func__) + MethodName );
}

void TJniObject::CallVoidMethod(const std::string& MethodName,const std::string& ParamA,TJniObject& ParamB)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<void>( MethodName, GetSignatureType<std::string>(), GetSignatureType(ParamB) );
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	auto ParamBj = ParamB.mObject;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj, ParamBj );
	ThrowJavaException( std::string(__func__) + MethodName );
}

void TJniObject::CallVoidMethod(const std::string& MethodName,int ParamA,bool ParamB)
{
	PreFunctionCall(__func__,MethodName,ParamA,ParamB);
	auto Method = GetMethod<void>( MethodName, GetSignatureType<int>(), GetSignatureType<bool>() );
	jint ParamAj = ParamA;
	jboolean ParamBj = ParamB;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj, ParamBj );
	ThrowJavaException( std::string(__func__) + MethodName );
}

void TJniObject::CallVoidMethod(const std::string& MethodName,TJniObject& ParamA,TJniObject& ParamB,TJniObject& ParamC,int ParamD)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<void>( MethodName, GetSignatureType(ParamA), GetSignatureType(ParamB), GetSignatureType(ParamC), GetSignatureType<int>() );
	auto ParamAj = ParamA.GetWeakObject();
	auto ParamBj = ParamB.GetWeakObject();
	auto ParamCj = ParamC.GetWeakObject();
	auto ParamDj = ParamD;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj, ParamBj, ParamCj, ParamDj );
	ThrowJavaException( std::string(__func__) + MethodName );
}

void TJniObject::CallVoidMethod(const std::string& MethodName,TJniObject& ParamA)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<void>( MethodName, GetSignatureType(ParamA) );
	auto ParamAj = ParamA.GetWeakObject();
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
}

void TJniObject::CallVoidMethod(const std::string& MethodName,int ParamA,int ParamB,int ParamC,long ParamD,int ParamE)
{
	PreFunctionCall(__func__,MethodName,ParamA,ParamB,ParamC,ParamD,ParamE);
	auto Method = GetMethod<void>(
								  MethodName,
								  GetSignatureType<int>(),
								  GetSignatureType<int>(),
								  GetSignatureType<int>(),
								  GetSignatureType<jlong>(),
								  GetSignatureType<int>()
								  );
	auto ParamAj = ParamA;
	auto ParamBj = ParamB;
	auto ParamCj = ParamC;
	jlong ParamDj = ParamD;
	auto ParamEj = ParamE;
	java().CallVoidMethod( GetWeakObject(), Method, ParamAj, ParamBj, ParamCj, ParamDj, ParamEj );
	ThrowJavaException( std::string(__func__) + MethodName );
}


bool TJniObject::CallBoolMethod(const std::string& MethodName)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<bool>( MethodName );
	auto Result = java().CallBooleanMethod( GetWeakObject(), Method );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}

uint8 TJniObject::CallByteMethod(const std::string& MethodName,const int& ParamA)
{
	PreFunctionCall(__func__,MethodName,ParamA);
	auto Method = GetMethod<jbyte>( MethodName, GetSignatureType<int>() );
	int ParamAj = ParamA;
	auto Result = java().CallByteMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}


int TJniObject::CallIntMethod(const std::string& MethodName)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<int>( MethodName );
	auto Result = java().CallIntMethod( GetWeakObject(), Method );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}


int TJniObject::CallIntMethod(const std::string& MethodName,const std::string& ParamA)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<int>( MethodName, GetSignatureType<jstring>() );
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	auto Result = java().CallIntMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}


int TJniObject::CallIntMethod(const std::string& MethodName,const int& ParamA)
{
	PreFunctionCall(__func__,MethodName,ParamA);
	auto Method = GetMethod<int>( MethodName, GetSignatureType<int>() );
	int ParamAj = ParamA;
	auto Result = java().CallIntMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}


int TJniObject::CallIntMethod(const std::string& MethodName,const long& ParamA)
{
	PreFunctionCall(__func__,MethodName,ParamA);
	auto Method = GetMethod<int>( MethodName, GetSignatureType<long>() );
	jlong ParamAj = ParamA;
	auto Result = java().CallIntMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}


int TJniObject::CallIntMethod(const std::string& MethodName,TJniObject& ParamA,const int& ParamB)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<int>( MethodName, GetSignatureType(ParamA), GetSignatureType<int>() );
	auto Result = java().CallIntMethod( GetWeakObject(), Method, ParamA.GetWeakObject(), ParamB );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}


int TJniObject::CallIntMethod(const std::string& MethodName,TJniObject& ParamA,const long& ParamB)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<int>( MethodName, GetSignatureType(ParamA), GetSignatureType<long>() );
	jlong ParamBj = ParamB;
	auto Result = java().CallIntMethod( GetWeakObject(), Method, ParamA.GetWeakObject(), ParamBj );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}


float TJniObject::CallFloatMethod(const std::string& MethodName,const std::string& ParamA)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<float>( MethodName, GetSignatureType<jstring>() );
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	auto Result = java().CallFloatMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}

jlong TJniObject::CallLongMethod(const std::string& MethodName)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<jlong>( MethodName );
	auto Result = java().CallLongMethod( GetWeakObject(), Method );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}

jlong TJniObject::CallLongMethod(const std::string& MethodName,const std::string& ParamA)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<jlong>( MethodName, GetSignatureType<jstring>() );
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	auto Result = java().CallLongMethod( GetWeakObject(), Method, ParamAj );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Result;
}

std::string TJniObject::CallStringMethod(const std::string& MethodName)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<jstring>( MethodName );
	//std::Debug << __func__ << " " << MethodName << std::endl;
	TJniLocalObject<jstring> ResultStr( java().CallObjectMethod( GetWeakObject(), Method ) );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Soy::JStringToString( ResultStr );
	//	release jstring here?
}

std::string TJniObject::CallStringMethod(const std::string& MethodName,const std::string& ParamA)
{
	PreFunctionCall(__func__,MethodName);
	auto Method = GetMethod<jstring>( MethodName, GetSignatureType<jstring>() );
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	TJniLocalObject<jstring> ResultStr( java().CallObjectMethod( GetWeakObject(), Method, ParamAj ) );
	ThrowJavaException( std::string(__func__) + MethodName );
	return Soy::JStringToString( ResultStr );
	//	release jstring here?
}

TJniObject TJniObject::CallObjectMethod(const std::string& MethodName,const std::string& ReturnClassName)
{
	PreFunctionCall(__func__,MethodName);
	TJniClass ReturnClass( ReturnClassName.c_str() );

	std::string Signature = GetSignature_ObjectReturn( ReturnClass );
	
	//std::Debug << __func__ << "::" << MethodName << std::endl;
	auto Method = GetMethod( MethodName, Signature );
	//	should throw exception before reaching here
	if ( !Method )
		throw Soy::AssertException("Method expected");

	//std::Debug << __func__ << " CallObjectMethod " << std::endl;
	auto ResultObjectj = java().CallObjectMethod( GetWeakObject(), Method );
	ThrowJavaException( std::string(__func__) + MethodName, ResultObjectj==nullptr );

	//	construct returning object
	//std::Debug << __func__ << " constructing return object... " << std::endl;
	TJniObject ResultObject( ResultObjectj, ReturnClass.GetWeakClass(), ReturnClass.GetClassName().c_str() );
	//std::Debug << __func__ << " returning..." << std::endl;
	return ResultObject;
}

TJniObject TJniObject::CallObjectMethod(const std::string& MethodName,const std::string& ReturnClassName,const std::string& ParamA)
{
	TJniString ParamAJString(ParamA);
	auto ParamAj = ParamAJString.mString;
	return CallObjectMethod( MethodName, ReturnClassName, ParamAj );
}

TJniObject TJniObject::CallObjectMethod(const std::string& MethodName,const std::string& ReturnClassName,const jstring& ParamA)
{
	PreFunctionCall(__func__,MethodName,ParamA);
	TJniClass ReturnClass( ReturnClassName.c_str() );
	
	std::string Signature = GetSignature_ObjectReturn( ReturnClass, GetSignatureType<jstring>() );

	auto Method = GetMethod( MethodName, Signature );
	auto ResultObjectj = java().CallObjectMethod( GetWeakObject(), Method, ParamA );
	ThrowJavaException( std::string(__func__) + MethodName, ResultObjectj==nullptr );
	
	//	construct returning object
	TJniObject ResultObject( ResultObjectj, ReturnClass.GetWeakClass(), ReturnClass.GetClassName().c_str() );
	return ResultObject;
}


TJniObject TJniObject::CallObjectMethod(const std::string& MethodName,const std::string& ReturnClassName,const int& ParamA)
{
	PreFunctionCall(__func__,MethodName,ParamA);
	TJniClass ReturnClass( ReturnClassName.c_str() );
	
	std::string Signature = GetSignature_ObjectReturn( ReturnClass, GetSignatureType<int>() );
	
	auto Method = GetMethod( MethodName, Signature );
	auto ResultObjectj = java().CallObjectMethod( GetWeakObject(), Method, ParamA );
	ThrowJavaException( std::string("CallObjectMethod( ") + MethodName + " )", ResultObjectj==nullptr );
	
	//	construct returning object
	TJniObject ResultObject( ResultObjectj, ReturnClass.GetWeakClass(), ReturnClass.GetClassName().c_str() );
	return ResultObject;
}



TJniObject TJniObject::CallObjectMethod(const std::string& MethodName,const std::string& ReturnClassName,const uint8& ParamA)
{
	PreFunctionCall(__func__,MethodName,ParamA);
	TJniClass ReturnClass( ReturnClassName.c_str() );
	
	std::string Signature = GetSignature_ObjectReturn( ReturnClass, GetSignatureType<uint8>() );
	
	auto Method = GetMethod( MethodName, Signature );
	auto ResultObjectj = java().CallObjectMethod( GetWeakObject(), Method, ParamA );
	ThrowJavaException( std::string("CallObjectMethod( ") + MethodName + " )", ResultObjectj==nullptr );
	
	//	construct returning object
	TJniObject ResultObject( ResultObjectj, ReturnClass.GetWeakClass(), ReturnClass.GetClassName().c_str() );
	return ResultObject;
}


TJniObject TJniClass::GetStaticFieldObject(const std::string& FieldName,const std::string& ClassName)
{
	//PreFunctionCall(__func__,FieldName);
	TJniClass FieldClass( ClassName.c_str() );
	
	//	field sig is just the type
	std::string Signature = GetSignatureType( FieldClass );

	//std::Debug << __func__ << "GetStaticFieldID(" << Signature << ")" << std::endl;
	auto FieldId = java().GetStaticFieldID( this->GetWeakClass(), FieldName.c_str(), Signature.c_str() );
	if ( !FieldId )
		ThrowJavaException(__func__,true);
	
	auto Objectj = java().GetStaticObjectField( this->GetWeakClass(), FieldId );
	if ( !Objectj )
		ThrowJavaException(__func__,true);
	
	TJniObject Object( Objectj, FieldClass.GetWeakClass(), FieldClass.GetClassName().c_str() );
	return Object;
}


void JniMediaPlayer::SetDataSourceAssets(const std::string& Path)
{
	//	trying to stream right out of assets
	std::Debug << "JniMediaPlayer::SetDataSourceAssets(" << Path << ")" << std::endl;
	
	//	load file descriptor from assets
	TJniString PathJString(Path);
	auto Filenamej = PathJString.mString;

	
	//	gotta get asset manager from context, we get that from the unity player
	TJniClass UnityPlayerClass("com.unity3d.player.UnityPlayer");
	TJniClass ActivityClass("android.app.Activity");
	TJniClass AssetManagerClass("android/content/res/AssetManager");
	//auto currentActivitySig = GetSignature_ObjectReturn(ActivityClass);
	//auto Method_CurrentActivity = UnityPlayerClass.GetStaticMethod("currentActivity", GetSignature_ObjectReturn(ActivityClass) );
	//auto Acitivityj = java().CallStaticObjectMethod( UnityPlayerClass.GetWeakClass(), Method_CurrentActivity );
	//TJniObject Activity( Acitivityj, ActivityClass.GetWeakClass() );
	
	std::Debug << "get static activity" << std::endl;
	TJniObject Activity = UnityPlayerClass.GetStaticFieldObject("currentActivity","android.app.Activity");
	std::Debug << "Activity has strong reference?" << (Activity.IsStrongObjectReference() ? "true":"false") << std::endl;
	
	std::Debug << "getAssets()" << std::endl;
	TJniObject AssetManager = Activity.CallObjectMethod("getAssets",AssetManagerClass.GetClassName());
	
	//std::Debug << "getting AssetManager..." << std::endl;
	//TJniObject AssetManager("android.content.res.AssetManager");
	std::Debug << "getting AssetFileDescriptor..." << std::endl;
	TJniClass AssetFileDescriptorClass("android/content/res/AssetFileDescriptor");
	
	std::Debug << "getting AssetManager.CallObjectMethod('openFd'),..." << std::endl;
	//	gr; this throws java.io.FileNotFoundException on large files (400mb bjork)
	TJniObject AssetFileDescriptor = AssetManager.CallObjectMethod("openFd", AssetFileDescriptorClass.GetClassName(), Filenamej );
	
	try
	{
		std::Debug << "getting FileDescriptor..." << std::endl;
		TJniObject FileDescriptor = AssetFileDescriptor.CallObjectMethod("getFileDescriptor","java/io/FileDescriptor");

		bool FileDescriptorIsValid = FileDescriptor.CallBoolMethod("valid");
		std::Debug << "File descriptor is valid: "  << (FileDescriptorIsValid?"true":"false") << std::endl;
		
		//	this suggests we should provide the FD lengths
		//	http://stackoverflow.com/a/9698300/355753
		//	gr: ^^^ doesn't load the file without FD offsets when reading from the APK/assets
		static bool ProvideFdLengths = true;
		if ( ProvideFdLengths )
		{
			auto FdOffset = AssetFileDescriptor.CallLongMethod("getStartOffset");
			auto FdLength = AssetFileDescriptor.CallLongMethod("getLength");
			
			//	set media source with file descriptor
			std::Debug << "getting SetDataSource(fd," << FdOffset << "," << FdLength << ") method..." << std::endl;
			auto Method_setDataSource = GetMethod<void>( "setDataSource", GetSignatureType(FileDescriptor), GetSignatureType<jlong>(), GetSignatureType<jlong>() );
			std::Debug << "calling SetDataSource..." << std::endl;
			java().CallVoidMethod( GetWeakObject(), Method_setDataSource, FileDescriptor.GetWeakObject(), FdOffset, FdLength );
			ThrowJavaException( std::string(__func__) + Path );
		}
		else
		{
			//	set media source with file descriptor
			std::Debug << "getting SetDataSource(fd) method..." << std::endl;
			auto Method_setDataSource = GetMethod<void>( "setDataSource", GetSignatureType(FileDescriptor) );
			std::Debug << "calling SetDataSource..." << std::endl;
			java().CallVoidMethod( GetWeakObject(), Method_setDataSource, FileDescriptor.GetWeakObject() );
			ThrowJavaException( std::string(__func__) + Path );
		}
		
		//	todo: close FD. According to docs we should close the fd immediately after SetDataSource returns
		//	http://developer.android.com/reference/android/media/MediaPlayer.html#setDataSource(android.content.Context, android.net.Uri)
		//	http://developer.android.com/reference/android/content/res/AssetFileDescriptor.html#close()
		AssetFileDescriptor.CallVoidMethod("close");
	}
	catch ( ... )
	{
		//	close FD here too!
		AssetFileDescriptor.CallVoidMethod("close");
		throw;
	}
}


void JniMediaPlayer::SetDataSourceSdCard(const std::string& Path)
{
	std::Debug << __func__ << std::endl;
	
	//	get the SD card path
	TJniClass EnvClass("android.os.Environment");
	TJniClass FileClass("java.io.File");
	auto ExternalStorageSig = GetSignature_ObjectReturn(FileClass);

	auto Method = EnvClass.GetStaticMethod("getExternalStorageDirectory", ExternalStorageSig );
	ThrowJavaException("android.os.Environment -> GetStaticMethod getExternalStorageDirectory()");
	
	auto ExternalPathj = java().CallStaticObjectMethod( EnvClass.GetWeakClass(), Method );
	ThrowJavaException("SetDataSourceSdCard: call getExternalStorageDirectory()");
	
	TJniObject ExternalPath( ExternalPathj, FileClass.GetWeakClass(), "java.io.File" );
	auto ExtPath = ExternalPath.CallStringMethod("getAbsolutePath");

	std::Debug << "Got sdcard path as: "  << ExtPath << std::endl;
	
	std::stringstream FullPath;
	FullPath << ExtPath << "/" << Path;
	SetDataSourcePath( FullPath.str() );
}


void JniMediaPlayer::SetDataSourcePath(const std::string& Path)
{
	CallVoidMethod( "setDataSource", Path );
}


void JniMediaPlayer::SetSurface(JSurface& Surface)
{
	auto Method = GetMethod<void>( "setSurface", GetSignatureType(Surface) );
	java().CallVoidMethod( GetWeakObject(), Method, Surface.GetStrongObject() );
	ThrowJavaException(__func__);
}

void JniMediaPlayer::Seek(SoyTime Time)
{
	//	need  valid duration before we can seek
	auto Duration = GetDuration();
	if ( Duration == 0 )
	{
		std::Debug << __func__ << "(" << Time << ") skipped as duration is " << Duration << std::endl;
		return;
	}
	
	auto SeekMs = size_cast<int>(Time.mTime);
	/*
	//	note; seeking to 0 seems to causes an error, but doesn't throw...
	if ( SeekMs == 0 )
	{
		std::Debug << __func__ << "(" << Time << ") skipped as is 0" << std::endl;
		return;
	}
	*/
	
	auto Method = GetMethod<void>( "seekTo", GetSignatureType<int>() );
	java().CallVoidMethod( GetWeakObject(), Method, SeekMs );
	ThrowJavaException(__func__);
}

void JniMediaPlayer::SetLooping(bool Looping)
{
	auto Method = GetMethod<void>( "setLooping", GetSignatureType<bool>() );
	java().CallVoidMethod( GetWeakObject(), Method, Looping );
	ThrowJavaException(__func__);
}

void JniMediaPlayer::SetTrack(int TrackIndex)
{
	auto Method = GetMethod<void>( "selectTrack", GetSignatureType<int>() );
	java().CallVoidMethod( GetWeakObject(), Method, TrackIndex );
	ThrowJavaException(__func__);
}


bool JSurfaceTexture::UpdateTexture()
{
	//	gr: lookout for dimension errors; http://developer.android.com/reference/android/graphics/SurfaceTexture.html#setDefaultBufferSize(int, int)
	auto Method = GetMethod<void>( "updateTexImage" );
	if ( !Method )
		return false;
	java().CallVoidMethod( GetWeakObject(), Method );
	ThrowJavaException(__func__);
	return true;
}

jlong JSurfaceTexture::GetTimestampNano()
{
	auto Method = GetMethod<jlong>( "getTimestamp" );
	if ( !Method )
		return false;
	auto Result = java().CallLongMethod( GetWeakObject(), Method );
	ThrowJavaException(__func__);
	return Result;
}

bool JSurfaceTexture::SetBufferSize(size_t Width,size_t Height)
{
	auto Method = GetMethod<void>( "setDefaultBufferSize", GetSignatureType<int>(), GetSignatureType<int>() );
	if ( !Method )
		return false;
	int w = size_cast<int>( Width );
	int h = size_cast<int>( Height );
	java().CallVoidMethod( GetWeakObject(), Method, w, h );
	ThrowJavaException(__func__);
	return true;
}



void JniMediaPlayer::GetState(std::ostream& Stream)
{
	bool Playing = IsPlaying();
	auto Time = GetCurrentTime();
	auto Duration = GetDuration();
	Stream << "media player " << (Playing?"Playing":"Not playing") << "; ";
	Stream << "time: " << Time << "/" << Duration << "ms; ";
	Stream << "size:" << GetWidth() << "x" << GetHeight() << "; ";
}


void JniMediaExtractor::SetDataSourceJar(const std::string& OrigPath)
{
	//	trying to stream right out of assets
	std::Debug << __func__ << "(" << OrigPath << ")" << std::endl;
	
	auto InternalFilenamePrefix = "!/";
	auto JarFilePrefix = "file://";
	
	//	correct filename
	//	gr: could put this prefix in the protocol, but just in case there are other methods, let our code attempt it
	std::string Path = OrigPath;
	if ( !Soy::StringTrimLeft( Path, JarFilePrefix, false ) )
		std::Debug << "Warning: Expected JAR filename (" << OrigPath << ") to begin with " << JarFilePrefix << std::endl;
	
	//	trim the suffix (path inside the jar/obb/zip)
	BufferArray<std::string,2> JarAndAsset;
	Soy::StringSplitByString( GetArrayBridge(JarAndAsset), Path, InternalFilenamePrefix, true );
	if ( JarAndAsset.GetSize() != 2 )
	{
		std::stringstream Error;
		Error << "Path did not split(x" << JarAndAsset.GetSize() << ") into filename" << InternalFilenamePrefix << "asset [" << Path << "]";
		throw Soy::AssertException( Error.str() );
	}
	
	//	specific error for missing zip resource class
	auto ZipResourceClass = "com.android.vending.expansion.zipfile.ZipResourceFile";
	try
	{
		TJniClass UnityPlayerClass(ZipResourceClass);
	}
	catch(std::exception& e)
	{
		std::stringstream Error;
		Error << "Error getting java class " << ZipResourceClass << ". Maybe need to add jar from android SDK (commonly built as zip_file.jar); " << e.what();
		//	gr: output to debug and re-throw instead?
		throw Soy::AssertException( Error.str() );
	}
	
	auto& JarFilename = JarAndAsset[0];
	auto& AssetFilename = JarAndAsset[1];
	
	TJniObject Container(ZipResourceClass,JarFilename);
	
	//	gr: returns a null object when filenot found (no exception!) so exception is forced on error
	TJniObject FileDescriptor = Container.CallObjectMethod("getAssetFileDescriptor", "android/content/res/AssetFileDescriptor", AssetFilename );
	
	std::Debug << "SetDataSourceAssetFileDescriptor" << std::endl;
	SetDataSourceAssetFileDescriptor( FileDescriptor, true );
}



void JniMediaExtractor::SetDataSourceAssets(const std::string& Path)
{
	//	trying to stream right out of assets
	std::Debug << "JniMediaExtractor::SetDataSourceAssets(" << Path << ")" << std::endl;
	
	//	load file descriptor from assets
	TJniString Filename( Path );
	auto Filenamej = Filename.mString;
	
	
	//	gotta get asset manager from context, we get that from the unity player
	TJniClass UnityPlayerClass("com.unity3d.player.UnityPlayer");
	TJniClass ActivityClass("android.app.Activity");
	TJniClass AssetManagerClass("android/content/res/AssetManager");
	//auto currentActivitySig = GetSignature_ObjectReturn(ActivityClass);
	//auto Method_CurrentActivity = UnityPlayerClass.GetStaticMethod("currentActivity", GetSignature_ObjectReturn(ActivityClass) );
	//auto Acitivityj = java().CallStaticObjectMethod( UnityPlayerClass.GetWeakClass(), Method_CurrentActivity );
	//TJniObject Activity( Acitivityj, ActivityClass.GetWeakClass() );
	
	std::Debug << "get static activity" << std::endl;
	TJniObject Activity = UnityPlayerClass.GetStaticFieldObject("currentActivity","android.app.Activity");
	std::Debug << "Activity has strong reference?" << (Activity.IsStrongObjectReference() ? "true":"false") << std::endl;
	
	std::Debug << "getAssets()" << std::endl;
	TJniObject AssetManager = Activity.CallObjectMethod("getAssets",AssetManagerClass.GetClassName());
	
	//std::Debug << "getting AssetManager..." << std::endl;
	//TJniObject AssetManager("android.content.res.AssetManager");
	std::Debug << "getting AssetFileDescriptor..." << std::endl;
	TJniClass AssetFileDescriptorClass("android/content/res/AssetFileDescriptor");
	
	std::Debug << "getting AssetManager.CallObjectMethod('openFd'),..." << std::endl;
	//	gr; this throws java.io.FileNotFoundException on large files (400mb bjork)
	TJniObject AssetFileDescriptor = AssetManager.CallObjectMethod("openFd", AssetFileDescriptorClass.GetClassName(), Filenamej );
	
	SetDataSourceAssetFileDescriptor( AssetFileDescriptor, true );
	
}


void JniMediaExtractor::SetDataSourceAssetFileDescriptor(TJniObject& AssetFileDescriptor,bool CloseOnFinish)
{
	try
	{
		std::Debug << "getting FileDescriptor..." << std::endl;
		TJniObject FileDescriptor = AssetFileDescriptor.CallObjectMethod("getFileDescriptor","java/io/FileDescriptor");
		
		bool FileDescriptorIsValid = FileDescriptor.CallBoolMethod("valid");
		std::Debug << "File descriptor is valid: "  << (FileDescriptorIsValid?"true":"false") << std::endl;
		
		auto FdOffset = AssetFileDescriptor.CallLongMethod("getStartOffset");
		auto FdLength = AssetFileDescriptor.CallLongMethod("getLength");
		
		//	set media source with file descriptor
		std::Debug << "getting SetDataSource(fd," << FdOffset << "," << FdLength << ") method..." << std::endl;
		auto Method_setDataSource = GetMethod<void>( "setDataSource", GetSignatureType(FileDescriptor), GetSignatureType<jlong>(), GetSignatureType<jlong>() );
		std::Debug << "calling SetDataSource..." << std::endl;
		java().CallVoidMethod( GetWeakObject(), Method_setDataSource, FileDescriptor.GetWeakObject(), FdOffset, FdLength );
		ThrowJavaException( "SetDataSourceAssetFileDescriptor setDataSource" );
		
		//	close FD. According to docs we should close the fd immediately after SetDataSource returns
		//	http://developer.android.com/reference/android/media/MediaPlayer.html#setDataSource(android.content.Context, android.net.Uri)
		//	http://developer.android.com/reference/android/content/res/AssetFileDescriptor.html#close()
		if ( CloseOnFinish )
			AssetFileDescriptor.CallVoidMethod("close");
	}
	catch ( ... )
	{
		//	close FD here too!
		if ( CloseOnFinish )
			AssetFileDescriptor.CallVoidMethod("close");
		throw;
	}
}


void JniMediaExtractor::SetDataSourcePath(const std::string& Path)
{
	//	gr: this is unreliable;
	//	http://stackoverflow.com/questions/23612250/mediaextractor-setdatasource-throws-ioexception-failed-to-instantiate-extractor
	//CallVoidMethod( "setDataSource", Path );
	
	Java::TRandomAccessFileHandle FileHandle(Path);
	Soy::Assert( FileHandle.mFileDescriptor!=nullptr, "File descriptor missing" );
	CallVoidMethod( "setDataSource", *FileHandle.mFileDescriptor );
}

/*

void JniMediaExtractor::SetDataSourceAssets(const std::string& Path)
{
	std::shared_ptr<Java::TApkFileHandle> pFileHandle( new Java::TApkFileHandle( Path ) );
	auto& FileHandle = *pFileHandle;
	auto& FileDescriptor = *FileHandle.mFileDescriptor;
	try
	{
		//	set media source with file descriptor
		std::Debug << "getting SetDataSource( " << FileDescriptor << "," << FileHandle.mFdOffset << "," << FileHandle.mFdLength << ") method..." << std::endl;
		auto Method_setDataSource = GetMethod<void>( "setDataSource", GetSignatureType(FileDescriptor), GetSignatureType<jlong>(), GetSignatureType<jlong>() );
		std::Debug << "calling SetDataSource..." << std::endl;
		java().CallVoidMethod( GetWeakObject(), Method_setDataSource, FileDescriptor.GetWeakObject(), FileHandle.mFdOffset, FileHandle.mFdLength );
		ThrowJavaException( std::string(__func__) + Path );
		
		//	todo: close FD. According to docs we should close the fd immediately after SetDataSource returns
		//	http://developer.android.com/reference/android/media/MediaPlayer.html#setDataSource(android.content.Context, android.net.Uri)
		//	http://developer.android.com/reference/android/content/res/AssetFileDescriptor.html#close()
		std::Debug << "resetting handle..." << std::endl;
		pFileHandle.reset();
	}
	catch ( ... )
	{
		std::Debug << "resetting handle after exception..." << std::endl;
		pFileHandle.reset();
		throw;
	}
}
*/



Java::TFileHandle::TFileHandle() :
	mFd					( INVALID_FILE_HANDLE ),
	mDoneInitialSeek	( false )
{
}


Java::TFileHandle::~TFileHandle()
{
	if ( mFd )
	{
		//	close( mFd );
		mFd = INVALID_FILE_HANDLE;
	}
}


void Java::TFileHandle::InitSeek()
{
	if ( mDoneInitialSeek )
		return;
	
	auto SeekPos = GetInitialSeekPos();
	auto Result = lseek( mFd, SeekPos, SEEK_SET );
	
	if ( Result == (off_t)-1 )
	{
		std::stringstream Error;
		Error << "Java file handle seek failed; " << Soy::Platform::GetLastErrorString();
		throw Soy::AssertException( Error.str() );
	}
	
	mDoneInitialSeek = true;
}


void Java::TFileHandle::Read(ArrayBridge<uint8>&& Data,bool& Eof)
{
	auto Fd = mFd;

	InitSeek();
	auto BytesRead = read( Fd, Data.GetArray(), Data.GetDataSize() );
	if ( BytesRead == -1 )
	{
		std::stringstream Error;
		Error << "Error reading file: " << Soy::Platform::GetLastErrorString();
		throw Soy::AssertException( Error.str() );
	}
	
	Soy::Assert( BytesRead >= 0, "Unexpected return from read()");
	
	if ( BytesRead == 0 )
	{
		Eof = true;
		Data.SetSize(0);
		return;
	}
	
	Data.SetSize( BytesRead );
}

	
	
Java::TApkFileHandle::TApkFileHandle(const std::string& OrigPath) :
	mFdOffset	( 0 ),
	mFdLength	( 0 )
{
	std::string Path = OrigPath;
	Soy::StringTrimLeft( Path, "apk:", false );
	
	//	trying to stream right out of assets
	std::Debug << "TApkFileHandle(" << Path << ")" << std::endl;
	
	//	load file descriptor from assets
	TJniString Filename( Path );
	auto Filenamej = Filename.mString;
	
	
	//	gotta get asset manager from context, we get that from the unity player
	TJniClass UnityPlayerClass("com.unity3d.player.UnityPlayer");
	TJniClass ActivityClass("android.app.Activity");
	TJniClass AssetManagerClass("android/content/res/AssetManager");
	//auto currentActivitySig = GetSignature_ObjectReturn(ActivityClass);
	//auto Method_CurrentActivity = UnityPlayerClass.GetStaticMethod("currentActivity", GetSignature_ObjectReturn(ActivityClass) );
	//auto Acitivityj = java().CallStaticObjectMethod( UnityPlayerClass.GetWeakClass(), Method_CurrentActivity );
	//TJniObject Activity( Acitivityj, ActivityClass.GetWeakClass() );
	
	std::Debug << "get static activity" << std::endl;
	TJniObject Activity = UnityPlayerClass.GetStaticFieldObject("currentActivity","android.app.Activity");
	std::Debug << "Activity has strong reference?" << (Activity.IsStrongObjectReference() ? "true":"false") << std::endl;
	
	std::Debug << "getAssets()" << std::endl;
	TJniObject AssetManager = Activity.CallObjectMethod("getAssets",AssetManagerClass.GetClassName());
	
	//std::Debug << "getting AssetManager..." << std::endl;
	//TJniObject AssetManager("android.content.res.AssetManager");
	std::Debug << "getting AssetFileDescriptor..." << std::endl;
	TJniClass AssetFileDescriptorClass("android/content/res/AssetFileDescriptor");
	
	std::Debug << "getting AssetManager.CallObjectMethod('openFd'),..." << std::endl;
	//	gr; this throws java.io.FileNotFoundException on large files (400mb bjork)
	mAssetFileDescriptor.reset( new TJniObject( AssetManager.CallObjectMethod("openFd", AssetFileDescriptorClass.GetClassName(), Filenamej ) ) );
	auto& AssetFileDescriptor = *mAssetFileDescriptor;
	
	std::Debug << "getting FileDescriptor..." << std::endl;
	mFileDescriptor.reset( new TJniObject( AssetFileDescriptor.CallObjectMethod("getFileDescriptor","java/io/FileDescriptor") ) );
	
	if ( mFileDescriptor )
	{
		auto& FileDescriptor = *mFileDescriptor;
		
		bool FileDescriptorIsValid = FileDescriptor.CallBoolMethod("valid");
		std::Debug << "File descriptor is valid: "  << (FileDescriptorIsValid?"true":"false") << std::endl;
		
		//	need to retrieve a private field to get actual file handle
		//	this may fail, but we still have the asset file descriptor for jni funcs that use it
		try
		{
			mFd = Java::GetField<int>( FileDescriptor, "descriptor" );
			std::Debug << "Extracted filedescriptor handle; " << mFd << std::endl;
		}
		catch(std::exception& e)
		{
			std::Debug << "Warning: Android OS does not expose file descriptor handle; " << e.what() << std::endl;
		}
	}
	
	mFdOffset = AssetFileDescriptor.CallLongMethod("getStartOffset");
	mFdLength = AssetFileDescriptor.CallLongMethod("getLength");
}

Java::TApkFileHandle::~TApkFileHandle()
{
	if ( mAssetFileDescriptor )
	{
		mAssetFileDescriptor->CallVoidMethod("close");
		mAssetFileDescriptor.reset();
	}
}




Java::TZipFileHandle::TZipFileHandle(const std::string& OrigPath) :
	mFdOffset	( 0 ),
	mFdLength	( 0 )
{
	std::string Path = OrigPath;
	Soy::StringTrimLeft( Path, "apk:", false );
	Soy::StringTrimLeft( Path, "jar:", false );
	
	auto InternalFilenamePrefix = "!/";
	auto JarFilePrefix = "file://";
	
	//	correct filename
	//	gr: could put this prefix in the protocol, but just in case there are other methods, let our code attempt it
	if ( !Soy::StringTrimLeft( Path, JarFilePrefix, false ) )
		std::Debug << "Warning: Expected JAR filename (" << OrigPath << ") to begin with " << JarFilePrefix << std::endl;
	
	//	trying to stream right out of assets
	std::Debug << __func__ << "(" << Path << ")" << std::endl;
	
	//	trim the suffix (path inside the jar/obb/zip)
	BufferArray<std::string,2> JarAndAsset;
	Soy::StringSplitByString( GetArrayBridge(JarAndAsset), Path, InternalFilenamePrefix, true );
	if ( JarAndAsset.GetSize() != 2 )
	{
		std::stringstream Error;
		Error << "Path did not split(x" << JarAndAsset.GetSize() << ") into filename" << InternalFilenamePrefix << "asset [" << Path << "]";
		throw Soy::AssertException( Error.str() );
	}
	
	//	specific error for missing zip resource class
	auto ZipResourceClass = "com.android.vending.expansion.zipfile.ZipResourceFile";
	try
	{
		TJniClass UnityPlayerClass(ZipResourceClass);
	}
	catch(std::exception& e)
	{
		std::stringstream Error;
		Error << "Error getting java class " << ZipResourceClass << ". Maybe need to add jar from android SDK (commonly built as zip_file.jar); " << e.what();
		//	gr: output to debug and re-throw instead?
		throw Soy::AssertException( Error.str() );
	}
	
	auto& JarFilename = JarAndAsset[0];
	auto& AssetFilename = JarAndAsset[1];
	
	TJniObject Container(ZipResourceClass,JarFilename);
	
	//	gr: returns a null object when filenot found (no exception!) so exception is forced on error
	TJniObject FileDescriptor = Container.CallObjectMethod("getAssetFileDescriptor", "android/content/res/AssetFileDescriptor", AssetFilename );

	mAssetFileDescriptor.reset( new TJniObject( FileDescriptor ) );
	auto& AssetFileDescriptor = *mAssetFileDescriptor;
	
	std::Debug << "getting FileDescriptor..." << std::endl;
	mFileDescriptor.reset( new TJniObject( AssetFileDescriptor.CallObjectMethod("getFileDescriptor","java/io/FileDescriptor") ) );
	
	if ( mFileDescriptor )
	{
		auto& FileDescriptor = *mFileDescriptor;
		
		bool FileDescriptorIsValid = FileDescriptor.CallBoolMethod("valid");
		std::Debug << "File descriptor is valid: "  << (FileDescriptorIsValid?"true":"false") << std::endl;
		
		//	need to retrieve a private field to get actual file handle
		//	this may fail, but we still have the asset file descriptor for jni funcs that use it
		try
		{
			mFd = Java::GetField<int>( FileDescriptor, "descriptor" );
			std::Debug << "Extracted filedescriptor handle; " << mFd << std::endl;
		}
		catch(std::exception& e)
		{
			std::Debug << "Warning: Android OS does not expose file descriptor handle; " << e.what() << std::endl;
		}
	}
	
	mFdOffset = AssetFileDescriptor.CallLongMethod("getStartOffset");
	mFdLength = AssetFileDescriptor.CallLongMethod("getLength");
}

	
Java::TZipFileHandle::~TZipFileHandle()
{
	if ( mAssetFileDescriptor )
	{
		mAssetFileDescriptor->CallVoidMethod("close");
		mAssetFileDescriptor.reset();
	}
}



Java::TRandomAccessFileHandle::TRandomAccessFileHandle(const std::string& Path)
{
	//	trying to stream right out of assets
	std::Debug << "Loading android file descriptor " << Path << std::endl;
	
	std::string Mode = "r";
	TJniObject RandomAccessFile("java.io.RandomAccessFile",Path,Mode);
	TJniObject Fd = RandomAccessFile.CallObjectMethod("getFD","java.io.FileDescriptor");
	mRandomAccessFile.reset( new TJniObject(RandomAccessFile) );
	mFileDescriptor.reset( new TJniObject( Fd ) );
	
	if ( mFileDescriptor )
	{
		auto& FileDescriptor = *mFileDescriptor;
		
		bool FileDescriptorIsValid = FileDescriptor.CallBoolMethod("valid");
		std::Debug << "File descriptor is valid: "  << (FileDescriptorIsValid?"true":"false") << std::endl;
		
		//	need to retrieve a private field to get actual file handle
		//	this may fail, but we still have the asset file descriptor for jni funcs that use it
		try
		{
			mFd = Java::GetField<int>( FileDescriptor, "descriptor" );
			std::Debug << "Extracted filedescriptor handle; " << mFd << std::endl;
		}
		catch(std::exception& e)
		{
			std::Debug << "Warning: Android OS does not expose file descriptor handle; " << e.what() << std::endl;
		}
	}
	
}


Java::TRandomAccessFileHandle::~TRandomAccessFileHandle()
{
	if ( mRandomAccessFile )
	{
		mRandomAccessFile->CallVoidMethod("close");
		mRandomAccessFile.reset();
	}
}

std::shared_ptr<Java::TFileHandle> Java::AllocFileHandle(const std::string& Filename)
{
	if ( Soy::StringContains( Filename, "!", true ) )
	{
		return std::make_shared<Java::TZipFileHandle>( Filename );
	}
	else if ( Soy::StringBeginsWith( Filename, "apk:", false ) )
	{
		return std::make_shared<Java::TApkFileHandle>( Filename );
	}
	else
	{
		return std::make_shared<Java::TRandomAccessFileHandle>( Filename );
	}
}



Java::TFileHandleStreamReader::TFileHandleStreamReader(const std::string& Filename,std::shared_ptr<TStreamBuffer> ReadBuffer) :
	TStreamReader	( std::string("java::TFileStreamReader ") + Filename, ReadBuffer ),
	mHandle			( AllocFileHandle( Filename ) )
{
}

Java::TFileHandleStreamReader::~TFileHandleStreamReader()
{
	WaitToFinish();
	mHandle.reset();
}

void Java::TFileHandleStreamReader::Read(TStreamBuffer& Buffer)
{
	if ( !mHandle )
		return;
	
	BufferArray<uint8,1024> Data(1024);
	bool Eof = false;
	mHandle->Read( GetArrayBridge(Data), Eof );
	
	Buffer.Push( GetArrayBridge(Data) );
}
