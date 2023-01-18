

#ifndef _SCOPEGUARD_H_
#define _SCOPEGUARD_H_



///////////////////////基类/////////////////////////////////////
class scope_guardImp
{
public:
	scope_guardImp() : _dismiss(false){}
	scope_guardImp(const scope_guardImp& other){
		_dismiss = other._dismiss;
		other.dismiss();
	}
public:
	void dismiss() const { _dismiss = true;}
protected:
	mutable bool _dismiss;
private:
	scope_guardImp& operator=(const scope_guardImp& other);
};

typedef const scope_guardImp& scope_guard;
////////////////////////0 参/////////////////////////////////
template<typename FUN>
class scope_guardS00 : public scope_guardImp
{
public:
	scope_guardS00(const FUN& fun)
		: _fun(fun){}

	~scope_guardS00()
	{
		if(!_dismiss){
			try{
				_fun();
			}catch(...){}
		}
	}
private:
	FUN  _fun;
};

////////////////////////1 参/////////////////////////////////
template<typename FUN, typename ARG>
class scope_guardS01 : public scope_guardImp
{
public:
	scope_guardS01(FUN fun,const ARG& arg)
		: _fun(fun),_arg(arg){}
	~scope_guardS01()
	{
		if(!_dismiss){
			try{
				_fun(_arg);
			}catch(...){}
		}
	}
private:
	FUN  _fun;
	const ARG  _arg;
};



////////////////////////////////////////////////////////////////////
//////////////////////////0 参//////////////////////////////////////
template<typename R,typename OBJ>
class scope_guardM00 : public scope_guardImp
{
public:
	scope_guardM00(R(OBJ::*fun)(),OBJ* obj)
		:_fun(fun),_obj(obj){}

	~scope_guardM00()
	{
		if(!_dismiss){
			try{
				(_obj->*_fun)();
			}catch(...){}
		}
	}
private:
	R(OBJ::*_fun)();
	OBJ*	_obj;
};

////////////////////////////1 参////////////////////////////////////
template<typename R, typename _ARG, typename OBJ, typename ARG>
class scope_guardM01 : public scope_guardImp
{
public:
	scope_guardM01(R(OBJ::*fun)(_ARG), OBJ* obj, const ARG& arg)
		:_fun(fun),_obj(obj),_arg(arg){}

	~scope_guardM01()
	{
		if(!_dismiss){
			try{
				(_obj->*_fun)(_arg);
			}catch(...){}
		}
	}
private:
	R(OBJ::*_fun)(_ARG);
	OBJ*	_obj;
	const ARG		_arg;
};



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

template<typename FUN>
scope_guardS00<FUN> make_guard(FUN fun){
	return scope_guardS00<FUN>(fun);
}

template<typename FUN,typename ARG>
scope_guardS01<FUN,ARG> make_guard(FUN fun,const ARG& arg){
	return scope_guardS01<FUN,ARG>(fun,arg);
}

template<typename R,typename OBJ>
scope_guardM00<R, OBJ> make_guard(R(OBJ::*fun)(), OBJ* obj){
	return scope_guardM00<R,OBJ>(fun,obj);
}

template<typename R, typename _ARG, typename OBJ, typename ARG>
scope_guardM01<R,_ARG,OBJ,ARG> make_guard(R(OBJ::*fun)(_ARG),OBJ *obj,const ARG& arg){
	return scope_guardM01<R, _ARG, OBJ, ARG>(fun, obj, arg);
}
#endif