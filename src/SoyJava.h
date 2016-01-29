#pragma once

#include <SoyAssert.h>
#include <SoyTime.h>
#include <SoyOpengl.h>
#include <jni.h>
#include <sstream>
#include <SoyStream.h>

class TJniObject;
class JSurfaceTexture;
class JSurface;
class TJniString;
class TJniClass;
template<typename TYPE>
class TJniLocalObject;

#define INVALID_FILE_HANDLE	0	//	gr: I swear this is declared somewhere in soy





namespace Soy
{
	std::string	JStringToString(jstring Stringj);
	std::string	JStringToString(TJniLocalObject<jstring>& Stringj);
}


namespace Java
{
	bool					HasVm();
	JNIEnv&					GetContext();
	void					FlushLocals();	//	wrapper to flush current thread's locals
	
	void					ArrayToBuffer(const ArrayBridge<uint8>&& Data,TJniObject& InputBuffer,const std::string& Context,int ExplicitBufferSize=-1);
	void					BufferToArray(TJniObject& InputBuffer,ArrayBridge<uint8>&& Data,const std::string& Context,int ExplicitBufferSize=-1);
	FixedRemoteArray<uint8>	GetBufferArray(TJniObject& Buffer,int LimitSize=-1);	//	get buffer as array. throws if this option isn't availible for this buffer
	void					IsOkay(const std::string& Context,bool ThrowRegardless=false);		//	check for JNI exception
	
	class TFileHandle;
	class TApkFileHandle;		//	special access to files in Assets which need to be loaded in a special way
	class TZipFileHandle;		//	expecting a filename like file://file.obb!/internalfilename.txt
	class TRandomAccessFileHandle;

	class TFileHandleStreamReader;	//	special file reader that uses JNI to read from APK
	typedef ::TFileStreamReader_ProtocolLambda<TFileHandleStreamReader> TApkFileStreamReader_ProtocolLambda;
	
	std::shared_ptr<TFileHandle>	AllocFileHandle(const std::string& Filename);

	//	according to android docs
	//	http://developer.android.com/training/articles/perf-jni.html
	//	JNI won't auto-free locals in a thread until it detatches (gr: not sure HOW it EVER cleans up then...)
	//	so instead, we'll manually push&pop frames of locals.
	//	hopefully this may show up cases where we need global refs, but more importantly we won't overflow the local stack (happens with long movies & ones with audio which use up locals much faster)
	//	gr: look for a way to auto count locals... not found one yet
	class TLocalRefStack;
	class TThread;			//	java thread handler, handles env creation & destruction and local object stack
}

namespace Platform
{
	std::string		GetSdCardDirectory();		//	throws if we can't get it
}


class Java::TLocalRefStack
{
public:
	TLocalRefStack(size_t MaxLocals=100);
	~TLocalRefStack();
};

class Java::TThread
{
public:
	TThread(JavaVM& vm);
	~TThread();
	
	void			Init()			{	FlushLocals();	}
	void			FlushLocals();

public:
	JavaVM&			mVirtualMachine;
	JNIEnv*			mThreadEnv;
	std::shared_ptr<TLocalRefStack>	mLocalStack;	//	ptr so we can destruct & recreate it to flush all local vars
};




class Java::TFileHandle
{
protected:
	static const int	UNKNOWN_LENGTH = -1;
public:
	TFileHandle();
	~TFileHandle();

	ssize_t			Seek();		//	returns bytes remaining, negative if unknown
	void			Read(ArrayBridge<uint8>&& Buffer,bool& Eof);

protected:
	virtual int		GetInitialSeekPos() const	{	return 0;	}
	virtual int		GetLength() const			{	return UNKNOWN_LENGTH;	}	//	-1 if unknown
	
public:
	int				mFd;
	std::shared_ptr<TJniObject>	mFileDescriptor;

protected:
	bool			mDoneInitialSeek;
};

class Java::TApkFileHandle : public Java::TFileHandle
{
public:
	TApkFileHandle(const std::string& Path);
	~TApkFileHandle();
	
	virtual int		GetInitialSeekPos() const override 	{	return mFdOffset;	}
	virtual int		GetLength() const override			{	return mFdLength;	}	//	-1 if unknown

protected:
	int				mFdOffset;
	int				mFdLength;
	
	std::shared_ptr<TJniObject>	mAssetFileDescriptor;
};


class Java::TZipFileHandle : public Java::TFileHandle
{
public:
	TZipFileHandle(const std::string& Path);
	~TZipFileHandle();
	
	virtual int		GetInitialSeekPos() const override 	{	return mFdOffset;	}
	virtual int		GetLength() const override			{	return mFdLength;	}	//	-1 if unknown
	
protected:
	int				mFdOffset;
	int				mFdLength;
	
	std::shared_ptr<TJniObject>	mAssetFileDescriptor;
};

class Java::TRandomAccessFileHandle : public Java::TFileHandle
{
public:
	TRandomAccessFileHandle(const std::string& Path);
	~TRandomAccessFileHandle();
	
	std::shared_ptr<TJniObject>	mRandomAccessFile;
};

class Java::TFileHandleStreamReader : public TStreamReader
{
public:
	TFileHandleStreamReader(const std::string& Filename,std::shared_ptr<TStreamBuffer> ReadBuffer=nullptr);
	~TFileHandleStreamReader();
	
protected:
	virtual bool		Read(TStreamBuffer& Buffer) override;
	
private:
	std::shared_ptr<Java::TFileHandle>	mHandle;
};




//	gr: work out signatures from javap
//	http://answers.unity3d.com/questions/857726/can-i-get-a-list-of-methods-for-a-class-with-jni.html#answer-984776
//	cd $ANDROID_HOME/platforms/android-19
//	javap -classpath android.jar -s android.media.MediaPlayer
template<typename TYPE>
inline std::string GetSignatureType(const TYPE& t)
{
	std::stringstream Signature;
	Soy::Assert( t.GetClassName().find('.') == std::string::npos, t.GetClassName() + "dots in class name when being used in signature. needs to be normalised" );
	Signature << 'L' << t.GetClassName() << ';';
	return Signature.str();
}

template<typename TYPE> inline std::string GetSignatureType()
{
	//	throw?
	return "NULL";
}

template<> inline std::string GetSignatureType<int>()	{	return "I";	}
template<> inline std::string GetSignatureType<bool>()	{	return "Z";	}
template<> inline std::string GetSignatureType<jlong>()	{	return "J";	}
template<> inline std::string GetSignatureType<long>()	{	return "J";	}
template<> inline std::string GetSignatureType<void>()	{	return "V";	}
template<> inline std::string GetSignatureType<float>()	{	return "F";	}
template<> inline std::string GetSignatureType<jbyte>()	{	return "B";	}
template<> inline std::string GetSignatureType<uint8>()	{	return "B";	}
template<> inline std::string GetSignatureType<jstring>()		{	return "Ljava/lang/String;";	}
template<> inline std::string GetSignatureType<std::string>()	{	return "Ljava/lang/String;";	}
template<> inline std::string GetSignatureType<TJniString>()	{	return "Ljava/lang/String;";	}



class TJniString : public NonCopyable
{
public:
	TJniString(const std::string& String);
	~TJniString();
	
	std::string	Get() const;
	
public:
	jstring		mString;
	std::string	mCache;		//	for debugging
};



class TJniParam
{
public:
	virtual const char*	GetSignature() const=0;
};

template<typename TYPE>
class TJniParam_TYPE : public TJniParam
{
public:
	TJniParam_TYPE(const TYPE& Value) :
		mValue	( Value )
	{
	}
	
	TYPE	mValue;
};

class TJniParam_object : public TJniParam_TYPE<jobject>
{
public:
	TJniParam_object(const char* Type,jobject Object) :
		mType			( Type ),
		TJniParam_TYPE	( Object )
	{
	}

	virtual const char*	GetSignature() const override	{	return mType;	}

	const char*	mType;
};


class TJniParam_int : public TJniParam_TYPE<int>
{
public:
	TJniParam_int(int Value) :
	TJniParam_TYPE	( Value )
	{
	}
	
	virtual const char*	GetSignature() const override	{	return "I";	}
};

/*
class TJniParam_string : public TJniParam_TYPE<jstring>
{
public:
	TJniParam_string(const std::string& Value) :
		TJniParam_TYPE	( Soy::StringToJString(Value) )
	{
	}
	
	virtual const char*	GetSignature() const override	{	return "Landroid/string;";	}
};
*/
template<typename RETURNTYPE>
inline std::string	GetSignature()
{
	std::stringstream ss;
	ss << "()" << GetSignatureType<RETURNTYPE>();
	return ss.str();
}

template<typename RETURNTYPE>
inline std::string	GetSignature(const std::string& SigTypea)
{
	std::stringstream ss;
	ss << "(" << SigTypea << ")" << GetSignatureType<RETURNTYPE>();
	return ss.str();
}

template<typename RETURNTYPE>
inline std::string	GetSignature(const std::string& SigTypea,const std::string& SigTypeb)
{
	std::stringstream ss;
	ss << "(" << SigTypea << SigTypeb << ")" << GetSignatureType<RETURNTYPE>();
	return ss.str();
}

template<typename RETURNTYPE>
inline std::string	GetSignature(const std::string& SigTypea,const std::string& SigTypeb,const std::string& SigTypec)
{
	std::stringstream ss;
	ss << "(" << SigTypea << SigTypeb << SigTypec << ")" << GetSignatureType<RETURNTYPE>();
	return ss.str();
}


template<typename RETURNTYPE>
inline std::string	GetSignature(const std::string& SigTypea,const std::string& SigTypeb,const std::string& SigTypec,const std::string& SigTyped)
{
	std::stringstream ss;
	ss << "(" << SigTypea << SigTypeb << SigTypec << SigTyped << ")" << GetSignatureType<RETURNTYPE>();
	return ss.str();
}


template<typename RETURNTYPE>
inline std::string	GetSignature(const std::string& SigTypea,const std::string& SigTypeb,const std::string& SigTypec,const std::string& SigTyped,const std::string& SigTypee)
{
	std::stringstream ss;
	ss << "(" << SigTypea << SigTypeb << SigTypec << SigTyped << SigTypee << ")" << GetSignatureType<RETURNTYPE>();
	return ss.str();
}


//	merge this into TJniObject
//	gr: use this to explicitly flush variables (eg. loops). For general clear up, use Java::TThread::FlushLocals()
template<typename TYPE>
class TJniLocalObject
{
public:
	//	construct with jobject to reduce neccessary casts in caller, and all objects should be derived from jobject anyway
	TJniLocalObject(jobject Object,bool ReleaseRef=true) :
		mObject		( static_cast<TYPE>(Object) ),
		mRelease	( ReleaseRef )
	{
	}
	~TJniLocalObject()
	{
		if ( mObject && mRelease )
		{
			auto& Context = Java::GetContext();
			Context.DeleteLocalRef( mObject );
			Java::IsOkay( "DeleteLocalRef" );
		}
	}
	
	operator	bool() const	{	return mObject != nullptr;	}
	TJniClass	GetClass();

public:
	TYPE		mObject;
	bool		mRelease;
};

class TJniClass
{
public:
	TJniClass(const char* ClassName);
	TJniClass(const jclass& Class,const char* ClassName);
	TJniClass(const TJniClass& Class);
	~TJniClass()	{	Release();	}
	
	operator	bool() const		{	return mClass != nullptr;	}
	TJniClass&	operator=(const TJniClass& that);

	const std::string&	GetClassName() const	{	return mClassName;	}
	static void			NormaliseClassName(std::string& ClassName);

	template<typename RETURNTYPE> jmethodID	GetMethod(const std::string& MethodName)	{	auto Signature = GetSignature<RETURNTYPE>();	return GetMethod( MethodName, Signature );	}
	template<typename RETURNTYPE> jmethodID	GetMethod(const std::string& MethodName,const std::string& SigTypea)	{	auto Signature = GetSignature<RETURNTYPE>(SigTypea);	return GetMethod( MethodName, Signature );	}
	template<typename RETURNTYPE> jmethodID	GetMethod(const std::string& MethodName,const std::string& SigTypea,const std::string& SigTypeb)	{	auto Signature = GetSignature<RETURNTYPE>(SigTypea,SigTypeb);	return GetMethod( MethodName, Signature );	}
	template<typename RETURNTYPE> jmethodID	GetMethod(const std::string& MethodName,const std::string& SigTypea,const std::string& SigTypeb,const std::string& SigTypec)	{	auto Signature = GetSignature<RETURNTYPE>(SigTypea,SigTypeb,SigTypec);	return GetMethod( MethodName, Signature );	}
	template<typename RETURNTYPE> jmethodID	GetMethod(const std::string& MethodName,const std::string& SigTypea,const std::string& SigTypeb,const std::string& SigTypec,const std::string& SigTyped)	{	auto Signature = GetSignature<RETURNTYPE>(SigTypea,SigTypeb,SigTypec,SigTyped);	return GetMethod( MethodName, Signature );	}
	template<typename RETURNTYPE> jmethodID	GetMethod(const std::string& MethodName,const std::string& SigTypea,const std::string& SigTypeb,const std::string& SigTypec,const std::string& SigTyped,const std::string& SigTypee)	{	auto Signature = GetSignature<RETURNTYPE>(SigTypea,SigTypeb,SigTypec,SigTyped,SigTypee);	return GetMethod( MethodName, Signature );	}

	jmethodID	GetMethod(const std::string& MethodName,const std::string& Signature);
	jmethodID	GetStaticMethod(const std::string& MethodName,const std::string& Signature);
	TJniObject	GetStaticFieldObject(const std::string& FieldName,const std::string& ClassName);

	TJniObject	CallStaticObjectMethod(const std::string& MethodName,const std::string& ReturnClass,const std::string& ParamA);
	TJniObject	CallStaticObjectMethod(const std::string& MethodName,const std::string& ReturnClass,const std::string& ParamA,const int& ParamB,const int& ParamC);
	TJniObject	CallStaticObjectMethod(const std::string& MethodName,const std::string& ReturnClass,int ParamA);

	
	jclass		GetWeakClass()		{	return mClass;	}
	void		Release();
	
protected:
	std::string		mClassName;
	jclass			mClass;
	bool			mAutoReleaseClass;
};

template<typename TYPE>
TJniClass TJniLocalObject<TYPE>::GetClass()
{
	auto& Context = Java::GetContext();
	return TJniClass( Context.GetObjectClass( mObject ), nullptr );
}



class TJniObject : public TJniClass
{
private:
	TJniObject(const TJniClass& Class) :
		mObject				( nullptr ),
		mAutoReleaseObject	( false ),
		TJniClass			( Class )
	{
	}
	
public:
	TJniObject(jobject Object,jclass Class,const char* ClassName);
	TJniObject(const char* ClassName);
//	TJniObject(const char* ClassName,const TJniParam& ParamA);
	
	//	sort these out so we don't have a thousand combos
	explicit TJniObject(const char* ClassName,const int& ParamA);
	explicit TJniObject(const char* ClassName,const int& ParamA,const bool& ParamB);
	explicit TJniObject(const char* ClassName,const std::string& ParamA);
	explicit TJniObject(const char* ClassName,const std::string& ParamA,const std::string& ParamB);
	explicit TJniObject(const char* ClassName,TJniObject& ParamA);
	TJniObject(const TJniObject& that);
	
	~TJniObject()	{	Release();	}

	//	gr: RAII implementation says no null objects... but sometimes for JNI we want to pass null. bit of a special case
	//		require class here?
	static TJniObject	Null(const std::string& ClassName)		{	return TJniObject( TJniClass(ClassName.c_str()) );	}
	
	operator	bool() const		{	return mObject != nullptr;	}
	TJniObject&	operator=(const TJniObject& that);
	
	jobject		GetWeakObject() const	{	return mObject;	}
	
	//	gr: only used as params to java funcs, should we get rid of this?
	jobject		GetStrongObject();
	
	void		Release();
	bool		IsStrongObjectReference() const	{	return mObject && mAutoReleaseObject;	}

	//	super simple cases - these throw on error
	void		CallVoidMethod(const std::string& MethodName);
	void		CallVoidMethod(const std::string& MethodName,const int& ParamA);
	void		CallVoidMethod(const std::string& MethodName,const long& ParamA);
	void		CallVoidMethod(const std::string& MethodName,int ParamA,bool ParamB);
	void		CallVoidMethod(const std::string& MethodName,TJniObject& ParamA);
	void		CallVoidMethod(const std::string& MethodName,TJniObject& ParamA,TJniObject& ParamB,TJniObject& ParamC,int ParamD);
	void		CallVoidMethod(const std::string& MethodName,int ParamA,int ParamB,int ParamC,long ParamD,int ParamE);
	void		CallVoidMethod(const std::string& MethodName,const std::string& ParamA);
	void		CallVoidMethod(const std::string& MethodName,const std::string& ParamA,const int& ParamB);
	void		CallVoidMethod(const std::string& MethodName,const std::string& ParamA,const long& ParamB);
	void		CallVoidMethod(const std::string& MethodName,const std::string& ParamA,TJniObject& ParamB);
	bool		CallBoolMethod(const std::string& MethodName);
	uint8		CallByteMethod(const std::string& MethodName,const int& ParamA);
	int			CallIntMethod(const std::string& MethodName);
	int			CallIntMethod(const std::string& MethodName,const std::string& ParamA);
	int			CallIntMethod(const std::string& MethodName,const int& ParamA);
	int			CallIntMethod(const std::string& MethodName,const long& ParamA);
	int			CallIntMethod(const std::string& MethodName,TJniObject& ParamA,const int& ParamB);
	int			CallIntMethod(const std::string& MethodName,TJniObject& ParamA,const long& ParamB);
	float		CallFloatMethod(const std::string& MethodName,const std::string& ParamA);
	jlong		CallLongMethod(const std::string& MethodName);
	jlong		CallLongMethod(const std::string& MethodName,const std::string& ParamA);
	std::string	CallStringMethod(const std::string& MethodName);
	std::string	CallStringMethod(const std::string& MethodName,const std::string& ParamA);
	TJniObject	CallObjectMethod(const std::string& MethodName,const std::string& ReturnClass);
	TJniObject	CallObjectMethod(const std::string& MethodName,const std::string& ReturnClass,const jstring& ParamA);	//	specific case atm
	TJniObject	CallObjectMethod(const std::string& MethodName,const std::string& ReturnClass,const std::string& ParamA);	//	specific case atm
	TJniObject	CallObjectMethod(const std::string& MethodName,const std::string& ReturnClass,const int& ParamA);		//	specific case atm
	TJniObject	CallObjectMethod(const std::string& MethodName,const std::string& ReturnClass,const uint8& ParamA);
	
	int			GetIntField(const std::string& Name);
	int			GetLongField(const std::string& Name);
	
	void		SetCallback(const std::string& SetListenerMethodName,std::function<void(void)> Callback);
	
private:
	void		Alloc(std::function<jobject()> Constructor);

	void		PreFunctionCall(const char* Func,const std::string& MethodName,...);
	
private:
	jobject	mObject;
	bool	mAutoReleaseObject;
};

std::ostream& operator<<(std::ostream &out,const TJniObject& in);




class JniMediaPlayer : public TJniObject
{
public:
	JniMediaPlayer() :
	TJniObject	( "android/media/MediaPlayer" )
	{
	}

	void		Prepare()			{	CallVoidMethod("prepare");	}
	void		Start()				{	CallVoidMethod("start");	}
	void		Stop()				{	CallVoidMethod("stop");	}
	void		Seek(SoyTime Time);
	void		SetLooping(bool Looping);
	void		SetTrack(int TrackIndex);
	bool		IsPlaying()			{	return CallBoolMethod("isPlaying");	}
	int			GetCurrentTime()	{	return CallIntMethod("getCurrentPosition");	}
	int			GetDuration()		{	return CallIntMethod("getDuration");	}
	int			GetWidth()			{	return CallIntMethod("getVideoWidth");	}
	int			GetHeight()			{	return CallIntMethod("getVideoHeight");	}
	
	void		SetDataSourceAssets(const std::string& Path);
	void		SetDataSourceSdCard(const std::string& Path);
	void		SetDataSourcePath(const std::string& Path);
	void		SetSurface(JSurface& Surface);
	
	void		GetState(std::ostream& Stream);
};


class JSurfaceTexture : public TJniObject
{
public:
	JSurfaceTexture(const Opengl::TTexture& Texture) :
	TJniObject	( "android/graphics/SurfaceTexture", Texture.mTexture.mName )
	{
		mMethodUpdateTexImage = GetMethod<void>("updateTexImage");
		mMethodGetTimestamp = GetMethod<jlong>("getTimestamp");
		mMethodSetDefaultBufferSize = GetMethod<void>("setDefaultBufferSize", GetSignatureType<int>(), GetSignatureType<int>() );
	}
	JSurfaceTexture(const Opengl::TTexture& Texture,bool SingleBufferMode) :
	TJniObject	( "android/graphics/SurfaceTexture", Texture.mTexture.mName, SingleBufferMode )
	{
		mMethodUpdateTexImage = GetMethod<void>("updateTexImage");
		mMethodGetTimestamp = GetMethod<jlong>("getTimestamp");
		mMethodSetDefaultBufferSize = GetMethod<void>("setDefaultBufferSize", GetSignatureType<int>(), GetSignatureType<int>() );
	}
	
	bool		UpdateTexture();
	jlong		GetTimestampNano();
	bool		SetBufferSize(size_t Width,size_t Height);
	
public:
	jmethodID	mMethodUpdateTexImage;
	jmethodID	mMethodGetTimestamp;
	jmethodID	mMethodSetDefaultBufferSize;
};

class JSurface : public TJniObject
{
public:
	JSurface(JSurfaceTexture& SurfaceTexture) :
		TJniObject	( "android/view/Surface", SurfaceTexture )
	{
	}
	
public:
};

class JniMediaFormat : public TJniObject
{
public:
	JniMediaFormat(const TJniObject& Object) :
		TJniObject	( Object )
	{
		Soy::Assert( GetWeakObject()!=nullptr, "JniMediaFormat expected object" );
	}
	
	
};

class JniMediaExtractor : public TJniObject
{
public:
	JniMediaExtractor() :
		TJniObject	( "android/media/MediaExtractor" )
	{
	}
	
	//	gr: change this to a file handle factory and let SetDataSource run off that
	void			SetDataSourceAssets(const std::string& Path);
	void			SetDataSourceJar(const std::string& Path);
	void			SetDataSourcePath(const std::string& Path);
	void			SetDataSourceAssetFileDescriptor(TJniObject& AssetFileDescriptor,bool CloseOnFinish=true);

	int				GetTrackCount()		{	return TJniObject::CallIntMethod("getTrackCount");	}
	JniMediaFormat	GetTrack(int TrackIndex)
	{
		auto Object = CallObjectMethod("getTrackFormat","android/media/MediaFormat", TrackIndex );
		return JniMediaFormat(Object);
	}
};


//	gr: where was this needed?
/*
template<>
inline std::string GetSignatureType(const TJniClass& t)
{
	return t.GetClassName().c_str();
}

*/
