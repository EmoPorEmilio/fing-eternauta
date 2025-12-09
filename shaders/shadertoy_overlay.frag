#version 450 core

// Port of the provided Shadertoy fragment to GLSL 450 core in a full-screen pass
// Inputs mapped:
// - iResolution: viewport size (xy) and 1.0 in z
// - iTime: seconds since start
// gl_FragCoord.xy used instead of fragCoord

out vec4 FragColor;

uniform vec3 iResolution;
uniform float iTime;
uniform float uSnowSpeed;
uniform float uSnowDirectionDeg;
uniform float uMotionBlur;  // 0.0 = no blur, higher = more trail

// --- Shadertoy code (adapted) ---
	vec2 mod289(vec2 x) {
	  return x - floor(x * (1.0 / 289.0)) * 289.0;
	}

	vec3 mod289(vec3 x) {
	  	return x - floor(x * (1.0 / 289.0)) * 289.0;
	}
	
	vec4 mod289(vec4 x) {
	  	return x - floor(x * (1.0 / 289.0)) * 289.0;
	}
	
	vec3 permute(vec3 x) {
	  return mod289(((x*34.0)+1.0)*x);
	}

	vec4 permute(vec4 x) {
	  return mod((34.0 * x + 1.0) * x, 289.0);
	}

	vec4 taylorInvSqrt(vec4 r)
	{
	  	return 1.79284291400159 - 0.85373472095314 * r;
	}
	
	float snoise(vec2 v)
	{
			const vec4 C = vec4(0.211324865405187,0.366025403784439,-0.577350269189626,0.024390243902439);
			vec2 i  = floor(v + dot(v, C.yy) );
			vec2 x0 = v -   i + dot(i, C.xx);
			
			vec2 i1;
			i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
			vec4 x12 = x0.xyxy + C.xxzz;
			x12.xy -= i1;
			
			i = mod289(i); // Avoid truncation effects in permutation
			vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
				+ i.x + vec3(0.0, i1.x, 1.0 ));
			
			vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
			m = m*m ;
			m = m*m ;
			
			vec3 x = 2.0 * fract(p * C.www) - 1.0;
			vec3 h = abs(x) - 0.5;
			vec3 ox = floor(x + 0.5);
			vec3 a0 = x - ox;
			
			m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
			
			vec3 g;
			g.x  = a0.x  * x0.x  + h.x  * x0.y;
			g.yz = a0.yz * x12.xz + h.yz * x12.yw;

			return 130.0 * dot(m, g);		
	}
	
	float cellular2x2(vec2 P)
	{
			#define K 0.142857142857 // 1/7
			#define K2 0.0714285714285 // K/2
			#define jitter 0.8 // jitter 1.0 makes F1 wrong more often
			
			vec2 Pi = mod(floor(P), 289.0);
			vec2 Pf = fract(P);
			vec4 Pfx = Pf.x + vec4(-0.5, -1.5, -0.5, -1.5);
			vec4 Pfy = Pf.y + vec4(-0.5, -0.5, -1.5, -1.5);
			vec4 p = permute(Pi.x + vec4(0.0, 1.0, 0.0, 1.0));
			p = permute(p + Pi.y + vec4(0.0, 0.0, 1.0, 1.0));
			vec4 ox = mod(p, 7.0)*K+K2;
			vec4 oy = mod(floor(p*K),7.0)*K+K2;
			vec4 dx = Pfx + jitter*ox;
			vec4 dy = Pfy + jitter*oy;
			vec4 d = dx * dx + dy * dy; // d11, d12, d21 and d22, squared
			// Sort out the two smallest distances
			
			// Cheat and pick only F1
			d.xy = min(d.xy, d.zw);
			d.x = min(d.x, d.y);
			return d.x; // F1 duplicated, F2 not computed
	}

	float fbm(vec2 p) {
 		   float f = 0.0;
    		float w = 0.5;
    		for (int i = 0; i < 5; i ++) {
					f += w * snoise(p);
					p *= 2.;
					w *= 0.5;
    		}
    		return f;
	}

// Helper function to compute snow at a given time
float computeSnow(vec2 uv, float time, vec2 dirUV, float speed) {
    vec2 GA = dirUV * (time * speed * uSnowSpeed);

    float A = (uv.x - (uv.y * 0.3));
    A = clamp(A, 0.0, 1.0);

    float F1 = 1.0 - cellular2x2((uv + (GA * 0.1)) * 8.0);
    float A1 = 1.0 - (A * 1.0);
    float N1 = smoothstep(0.998, 1.0, F1) * 1.0 * A1;

    float F2 = 1.0 - cellular2x2((uv + (GA * 0.2)) * 6.0);
    float A2 = 1.0 - (A * 0.8);
    float N2 = smoothstep(0.995, 1.0, F2) * 0.85 * A2;

    float F3 = 1.0 - cellular2x2((uv + (GA * 0.4)) * 4.0);
    float A3 = 1.0 - (A * 0.6);
    float N3 = smoothstep(0.99, 1.0, F3) * 0.65 * A3;

    float F4 = 1.0 - cellular2x2((uv + (GA * 0.6)) * 3.0);
    float A4 = 1.0 - (A * 1.0);
    float N4 = smoothstep(0.98, 1.0, F4) * 0.4 * A4;

    float F5 = 1.0 - cellular2x2((uv + (GA)) * 1.2);
    float A5 = 1.0 - (A * 1.0);
    float N5 = smoothstep(0.98, 1.0, F5) * 0.25 * A5;

    return clamp(N1 + N2 + N3 + N4 + N5, 0.0, 1.0);
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    float speed = 2.0;
    vec2 uv = fragCoord / iResolution.xy;
    uv.x *= (iResolution.x / iResolution.y);

    float ang = radians(uSnowDirectionDeg);
    vec2 dir = normalize(vec2(cos(ang), sin(ang)));
    vec2 dirUV = vec2(dir.x * (iResolution.y / iResolution.x), dir.y);

    float SnowMask = 0.0;

    if (uMotionBlur <= 0.0) {
        // No motion blur - single sample
        SnowMask = computeSnow(uv, iTime, dirUV, speed);
    } else {
        // Motion blur - sample multiple time steps and blend
        const int BLUR_SAMPLES = 8;
        float timeStep = uMotionBlur * 0.02;  // Time span to blur over
        float totalWeight = 0.0;

        for (int i = 0; i < BLUR_SAMPLES; ++i) {
            float t = float(i) / float(BLUR_SAMPLES - 1);  // 0 to 1
            float sampleTime = iTime - t * timeStep;
            float weight = 1.0 - t * 0.5;  // Fade older samples
            SnowMask += computeSnow(uv, sampleTime, dirUV, speed) * weight;
            totalWeight += weight;
        }
        SnowMask /= totalWeight;
    }

    vec3 snowColor = vec3(0.9, 1.0, 1.1);
    FragColor = vec4(snowColor, SnowMask);
}


