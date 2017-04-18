import c3py
import numpy as np


def func1(x):
    return np.sum(x,axis=1)

def func2(x):
    # return np.prod(x-1,axis=1)
    return np.log10(np.sum(x,axis=1)**2)

dim = 25
ndata = 500
# x = np.random.rand(ndata,dim)*2.0-1.0
x = np.random.randn(ndata,dim)
y1 = func1(x)
y2 = func2(x)

lb = -1
ub = 1
nparam = 4

ft = c3py.FunctionTrain(dim)
for ii in range(dim):
    ft.set_dim_opts(ii,"legendre",lb,ub,nparam)
    # ft.set_dim_opts(ii,"hermite",nparam)

# ft.set_ranks(ranks)
ft.build_data_model(ndata,x,y1,alg="AIO",obj="LS",adaptrank=1,kickrank=1,roundtol=1e-10,verbose=0)

ft2 = c3py.FunctionTrain(dim)
for ii in range(dim):
    ft2.set_dim_opts(ii,"legendre",lb,ub,nparam)
    # ft2.set_dim_opts(ii,"hermite",nparam)

ft2.build_data_model(ndata,x,y2,alg="AIO",obj="LS",adaptrank=1,kickrank=1,roundtol=1e-10,verbose=1)

ft3 = ft + ft2
ft4 = ft * ft2

test_pt = np.random.rand(dim)*2.0-1.0
ft2eval = ft2.eval(test_pt)
ft1eval = ft.eval(test_pt)
ft3eval = ft3.eval(test_pt)
ft4eval = ft4.eval(test_pt)
eval1s = func1(test_pt.reshape((1,dim)))
eval2s = func2(test_pt.reshape((1,dim)))
eval3s = eval1s + eval2s
eval4s = eval1s * eval2s

print("Fteval =",ft1eval, "Should be =",eval1s)
print("Fteval =",ft2eval, "Should be =",eval2s)
print("Fteval =",ft3eval, "Should be =",eval3s)
print("Fteval =",ft4eval, "Should be =",eval4s)


ft.close()
ft2.close()
ft3.close()
ft4.close()
# mr = c3.function_train_get_maxrank(ft)


# print("max rank", mr)
# print("Integral ", c3.function_train_integrate(ft), "Should be = 8")
