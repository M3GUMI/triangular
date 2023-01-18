
#ifndef _CLOSURE_H_
#define _CLOSURE_H_


///////////////接口/////////////////////
class Closure
{
public:
	Closure(void){}
	virtual ~Closure(void){}

	virtual void Run() = 0;
};

/////////////////////////////////////////////////////////////
//////////		普通函数
///////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
//////////	0 参数，非成员函数
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename FUNC>
Closure* NewClosure(FUNC _fun){
	return new ClosureS00<FUNC>(_fun);
}

template<typename FUNC>
class ClosureS00 : public Closure
{
public:
	ClosureS00(const FUNC& _fun)
		: m_fun(_fun){}
	virtual void Run(){
		(*m_fun)();
	}
private:
	FUNC m_fun ;
};

///////////////////////////////////////////////////////////////////////////////////////////
//////////	1 参数，非成员函数
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename FUNC,typename ARG1>
Closure* NewClosure(FUNC _fun,ARG1 arg1){
	return new ClosureS01<FUNC,ARG1>(_fun,arg1);
}

template<typename FUNC,typename ARG1>
class ClosureS01 : public Closure
{
public:
	ClosureS01(const FUNC& _fun, const ARG1& _arg1)
		: m_fun(_fun)
		,m_arg1(_arg1){}
	virtual void Run(){
		(*m_fun)(m_arg1);
	}
private:
	FUNC m_fun ;
	const ARG1 m_arg1;
};

///////////////////////////////////////////////////////////////////////////////////////////
//////////	2 参数，非成员函数
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename FUNC,typename ARG1,typename ARG2>
Closure* NewClosure(const FUNC& _fun, const ARG1& arg1, const ARG2& arg2){
	return new ClosureS02<FUNC,ARG1,ARG2>(_fun,arg1,arg2);
}

template<typename FUNC,typename ARG1,typename ARG2>
class ClosureS02 : public Closure
{
public:
	ClosureS02(const FUNC& _fun, const ARG1& _arg1, const ARG2& _arg2)
		: m_fun(_fun)
		,m_arg1(_arg1)
		,m_arg2(_arg2){}
	virtual void Run(){
		(*m_fun)(m_arg1,m_arg2);
	}
private:
	FUNC m_fun ;
	const ARG1 m_arg1;
	const ARG2 m_arg2;
};

///////////////////////////////////////////////////////////////////////////////////////////
//////////	3 参数，非成员函数
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename FUNC, typename ARG1, typename ARG2, typename ARG3>
Closure* NewClosure(const FUNC& _fun, const ARG1& arg1, const ARG2& arg2, const ARG3& arg3){
	return new ClosureS03<FUNC, ARG1, ARG2,ARG3>(_fun, arg1, arg2,arg3);
}

template<typename FUNC, typename ARG1, typename ARG2, typename ARG3>
class ClosureS03 : public Closure
{
public:
	ClosureS03(const FUNC& _fun, const ARG1& _arg1, const ARG2& _arg2, const ARG3& _arg3)
		: m_fun(_fun)
		, m_arg1(_arg1)
		, m_arg2(_arg2)
		, m_arg3(_arg3){}
	virtual void Run(){
		(*m_fun)(m_arg1, m_arg2,m_arg3);
	}
private:
	FUNC m_fun;
	const ARG1 m_arg1;
	const ARG2 m_arg2;
	const ARG3 m_arg3;
};

/////////////////////////////////////////////////////
/////	类成员函数
///////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
//////////	0 参数，成员函数
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename R, typename OBJ>
Closure* NewClosure(R(OBJ::*fun)(), OBJ *obj){
	return new ClosureM00<R,OBJ>(fun,obj);
}

template<typename R, typename OBJ>
class ClosureM00 : public Closure
{
public:
	ClosureM00(R(OBJ::*_fun)(), OBJ* _obj)
		: m_fun(_fun)
		,m_obj(_obj){}
	virtual void Run(){
		(m_obj->*m_fun)();
	}
private:
	R(OBJ::*m_fun)();
	OBJ* m_obj;
};


///////////////////////////////////////////////////////////////////////////////////////////
//////////	1 参数，成员函数
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename R, typename _ARG1,typename OBJ,typename ARG1>
Closure* NewClosure(R(OBJ::*fun)(_ARG1), OBJ *obj, const ARG1& arg1){
	return new ClosureM01<R,_ARG1,OBJ,ARG1>(fun,obj,arg1);
}

template<typename R, typename _ARG1, typename OBJ, typename ARG1>
class ClosureM01 : public Closure
{
public:
	ClosureM01(R(OBJ::*_fun)(_ARG1), OBJ* _obj, const ARG1& _arg1)
		: m_fun(_fun)
		,m_obj(_obj)
		,m_arg1(_arg1){}
	virtual void Run(){
		(m_obj->*m_fun)(m_arg1);
	}
private:
	R(OBJ::*m_fun)(_ARG1);
	OBJ* m_obj;
	const ARG1 m_arg1;
};

///////////////////////////////////////////////////////////////////////////////////////////
//////////	2 参数，成员函数
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename R, typename _ARG1, typename _ARG2, typename OBJ, typename ARG1, typename ARG2>
Closure* NewClosure(R(OBJ::*fun)(_ARG1,_ARG2), OBJ *obj, const ARG1& arg1, const ARG2& arg2){
	return new ClosureM02<FUNC,OBJ,ARG1,ARG2>(fun,obj,arg1,arg2);
}

template<typename R, typename _ARG1, typename _ARG2, typename OBJ, typename ARG1, typename ARG2>
class ClosureM02 : public Closure
{
public:
	ClosureM02(R(OBJ::*_fun)(_ARG1, _ARG2), OBJ* _obj, const ARG1& _arg1, const ARG2& _arg2)
		: m_fun(_fun)
		,m_obj(_obj)
		,m_arg1(_arg1)
		,m_arg2(_arg2){}
	virtual void Run(){
		(m_obj->*m_fun)(m_arg1,m_arg2);
	}
private:
	R(OBJ::*m_fun)(_ARG1, _ARG2);
	OBJ* m_obj;
	const ARG1 m_arg1;
	const ARG2 m_arg2;
};

#endif