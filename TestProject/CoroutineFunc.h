#ifndef COROUTINE_FUNC_H_
#define COROUTINE_FUNC_H_


#define coroutineBegin(...) static unsigned int state = 0; switch(state) { case 0:
#define yieldReturn(...) {state = __LINE__; return __VA_ARGS__; case __LINE__:;}
#define coroutineEnd(...) state = __LINE__; case __LINE__:; return __VA_ARGS__;} return __VA_ARGS__


// In your coroutine you have to register as many states as many yield macros you
// use. State registration allows the coroutine to resume at the exact point where
// it last returned.
#define REG_ST(stateVariableName, stateIndex) if (stateVariableName == stateIndex) goto state##stateIndex;
#define REGISTER_STATES_1(stateVariableName) REG_ST(stateVariableName, 1)
#define REGISTER_STATES_2(stateVariableName) REGISTER_STATES_1(stateVariableName) REG_ST(stateVariableName, 2)
#define REGISTER_STATES_3(stateVariableName) REGISTER_STATES_2(stateVariableName) REG_ST(stateVariableName, 3)
#define REGISTER_STATES_4(stateVariableName) REGISTER_STATES_3(stateVariableName) REG_ST(stateVariableName, 4)
#define REGISTER_STATES_5(stateVariableName) REGISTER_STATES_4(stateVariableName) REG_ST(stateVariableName, 5)
#define REGISTER_STATES_6(stateVariableName) REGISTER_STATES_5(stateVariableName) REG_ST(stateVariableName, 6)
#define REGISTER_STATES_7(stateVariableName) REGISTER_STATES_6(stateVariableName) REG_ST(stateVariableName, 7)
#define REGISTER_STATES_8(stateVariableName) REGISTER_STATES_7(stateVariableName) REG_ST(stateVariableName, 8)
#define REGISTER_STATES_9(stateVariableName) REGISTER_STATES_8(stateVariableName) REG_ST(stateVariableName, 9)
#define REGISTER_STATES_10(stateVariableName) REGISTER_STATES_9(stateVariableName) REG_ST(stateVariableName, 10)
#define REGISTER_STATES_11(stateVariableName) REGISTER_STATES_10(stateVariableName) REG_ST(stateVariableName, 11)
#define REGISTER_STATES_12(stateVariableName) REGISTER_STATES_11(stateVariableName) REG_ST(stateVariableName, 12)
#define REGISTER_STATES_13(stateVariableName) REGISTER_STATES_12(stateVariableName) REG_ST(stateVariableName, 13)
#define REGISTER_STATES_14(stateVariableName) REGISTER_STATES_13(stateVariableName) REG_ST(stateVariableName, 14)
#define REGISTER_STATES_15(stateVariableName) REGISTER_STATES_14(stateVariableName) REG_ST(stateVariableName, 15)
#define REGISTER_STATES_16(stateVariableName) REGISTER_STATES_15(stateVariableName) REG_ST(stateVariableName, 16)
#define REGISTER_STATES_17(stateVariableName) REGISTER_STATES_16(stateVariableName) REG_ST(stateVariableName, 17)
#define REGISTER_STATES_18(stateVariableName) REGISTER_STATES_17(stateVariableName) REG_ST(stateVariableName, 18)
#define REGISTER_STATES_19(stateVariableName) REGISTER_STATES_18(stateVariableName) REG_ST(stateVariableName, 19)
#define REGISTER_STATES_20(stateVariableName) REGISTER_STATES_19(stateVariableName) REG_ST(stateVariableName, 20)


// Specifies coroutine prototype and the name of the state
// variable which should be defined (recommended as not public)
// in the class/structure providing the coroutine. State 0
// is by default meant to denote the begining of the coroutine
// while -1 means that the coroutine reached the end - thus
// state variable should be a signed integral type (int recommended).
#define COROUTINE_FUNC(stateVariableName, methodPrototype) \
	methodPrototype \
	{ \
		if (stateVariableName == -1) goto finished; 


// Returns a value or nothing (specified in __VA_ARGS__ argument)
// and saves coroutine state in specified stateVariableName.
#define YIELD(stateVariableName, stateIndex, ...) \
stateVariableName = stateIndex; \
return __VA_ARGS__; \
stateVariableName##stateIndex:


// Denotes the end of the coroutine. The argument specified in
// __VA_ARGS__ is the value returned when the coroutine
// reaches its end. Next coroutine calls after reaching this
// point will always return this value unless the state variable
// is reset to 0. Coroutine "finished" state is -1.
#define COROUTINE_ENDFUNC(stateVariableName, ...) \
stateVariableName = -1; \
finished: \
return __VA_ARGS__; \
}


#define COROUTINE(returnType, ...) \
public: \
	returnType operator()(__VA_ARGS__) \
	{ \
		if (state == -1) goto finished; 


#define YIELD_RETURN(stateIndex, ...) \
state = stateIndex; \
return __VA_ARGS__; \
state##stateIndex:


#define COROUTINE_END(...) \
state = -1; \
finished: \
return __VA_ARGS__; \
}


/*==================================================
Example coroutine:

class SimpleCoroutine
{
	int state = 0;
	SomeType myLocalVariable;
	int i;

	public:
		COROUTINE_FUNC(state, std::string operator()())
			REGISTER_STATES_4(state)

			YIELD(state, 1, "This is a string.")
			YIELD(state, 2, "I return for the second time!")

			// use myLocalVariable somewhere in the coroutine;
			// its value is remembered between the calls

			// i is a member variable
			for (i = 0; i < 10; i++)
			{
				// loop body
			}

			YIELD(state, 3, "Uh-oh...")
			YIELD(state, 4, "What more should I say?!")
		COROUTINE_ENDFUNC(state, "Nothing else other than \"nothing else\" shall I say!!")
};


Which can be later used as follows:

SimpleCoroutine myCoroutine;
myCoroutine();	// returns "This is a string."
myCoroutine();	// returns "I return for the second time!"

It is worth noting that every local variable in the coroutine MUST
be declared as a class/struct member variable in order to make its
value remember between the coroutine calls.
==================================================*/


#endif