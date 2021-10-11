/*	@(#)lib.b 1.1 92/07/30 SMI; from UCB 4.1 83/04/02	*/

scale = 20
define e(x){
	auto a, b, c, d, e, g, t, w, y

	t = scale
	scale = t + .434*x + 1

	w = 0
	if(x<0){
		x = -x
		w = 1
	}
	y = 0
	while(x>2){
		x = x/2
		y = y + 1
	}

	a=1
	b=1
	c=b
	d=1
	e=1
	for(a=1;1==1;a++){
		b=b*x
		c=c*a+b
		d=d*a
		g = c/d
		if(g == e){
			g = g/1
			while(y--){
				g = g*g
			}
			scale = t
			if(w==1) return(1/g)
			return(g/1)
		}
		e=g
	}
}

define l(x){
	auto a, b, c, d, e, f, g, u, s, t
	if(x <=0) return(1-10^scale)
	t = scale

	f=1
	scale = scale + scale(x) - length(x) + 1
	s=scale
	while(x > 2){
		s = s + (length(x)-scale(x))/2 + 1
		if(s>0) scale = s
		x = sqrt(x)
		f=f*2
	}
	while(x < .5){
		s = s + (length(x)-scale(x))/2 + 1
		if(s>0) scale = s
		x = sqrt(x)
		f=f*2
	}

	scale = t + length(f) - scale(f) + 1
	u = (x-1)/(x+1)

	scale = scale + 1.1*length(t) - 1.1*scale(t)
	s = u*u
	b = 2*f
	c = b
	d = 1
	e = 1
	for(a=3;1==1;a=a+2){
		b=b*s
		c=c*a+d*b
		d=d*a
		g=c/d
		if(g==e){
			scale = t
			return(u*c/d)
		}
		e=g
	}
}

define s(x){
	auto a, b, c, s, t, y, p, n, i
	t = scale
	y = x/.7853
	s = t + length(y) - scale(y)
	if(s<t) s=t
	scale = s
	p = a(1)

	scale = 0
	if(x>=0) n = (x/(2*p)+1)/2
	if(x<0) n = (x/(2*p)-1)/2
	x = x - 4*n*p
	if(n%2!=0) x = -x

	scale = t + length(1.2*t) - scale(1.2*t)
	y = -x*x
	a = x
	b = 1
	s = x
	for(i=3; 1==1; i=i+2){
		a = a*y
		b = b*i*(i-1)
		c = a/b
		if(c==0){scale=t; return(s/1)}
		s = s+c
	}
}

define c(x){
	auto t
	t = scale
	scale = scale+1
	x = s(x+2*a(1))
	scale = t
	return(x/1)
}

define a(x){
	auto a, b, c, d, e, f, g, s, t
	if(x==0) return(0)
	if(x==1) {
		if(scale<52) {
return(.7853981633974483096156608458198757210492923498437764/1)
		}
	}
	t = scale
	f=1
	while(x > .5){
		scale = scale + 1
		x= -(1-sqrt(1.+x*x))/x
		f=f*2
	}
	while(x < -.5){
		scale = scale + 1
		x = -(1-sqrt(1.+x*x))/x
		f=f*2
	}
	s = -x*x
	b = f
	c = f
	d = 1
	e = 1
	for(a=3;1==1;a=a+2){
		b=b*s
		c=c*a+d*b
		d=d*a
		g=c/d
		if(g==e){
			scale = t
			return(x*c/d)
		}
		e=g
	}
}

define j(n,x){
auto a,b,c,d,e,g,i,s,k,t

	t = scale
	k = 1.36*x + 1.16*t - n
	k = length(k) - scale(k)
	if(k>0) scale = scale + k

s= -x*x/4
if(n<0){
	n= -n
	x= -x
	}
a=1
c=1
for(i=1;i<=n;i++){
	a=a*x
	c = c*2*i
	}
b=a
d=1
e=1
for(i=1;1;i++){
	a=a*s
	b=b*i*(n+i) + a
	c=c*i*(n+i)
	g=b/c
	if(g==e){
		scale = t
		return(g/1)
		}
	e=g
	}
}
