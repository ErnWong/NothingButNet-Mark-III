typedef void (*Function)(void *)

typedef struct Lambda
{
    void * args;
    Function function;
}
Lambda;

Lambda *
lambaInit(Function function, void * arguments)
{
    Lambda lambda = malloc(sizeof(Labmda));
    lambda
}

// todo: lambda vs lamda?
