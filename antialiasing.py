def load_ppm(filename):
    with open(filename, "r") as f:
        f.readline()
        f.readline()
        (width, height) = map(int, f.readline().split(" "))
        max_rgb = int(f.readline())
        print(width, height, max_rgb)
        image = []
        i = 0
        j = 0
        k = 0
        for line in f.readlines():
            if line[0] == '#': continue
            if j == 0 and k == 0: image.append([])
            if k == 0: image[i].append([])
            image[i][j].append(int(line))
            k += 1
            if k == 3:
                k = 0
                j += 1
                if j == width:
                    j = 0
                    i += 1
        return image

def save_ppm(filename, image):
    with open(filename, "w") as f:
        f.write("P3\n")
        f.write("# CREATOR: Python antialiaser\n")
        f.write(" ".join([str(len(image[0])), str(len(image))]) + "\n")
        f.write(str(255) + "\n")
        for row in image:
            for col in row:
                for chn in col:
                    f.write(str(chn) + "\n")

def mlaa(img):
    #find discontinuities between pixels in a given image
    #identify U-shaped, Z-shaped and L-shaped patterns
    #blend colours in the neighbourhood of these patterns

    #step 1 - discontinuities
    CONTRAST_PER_CHN = 16
    check = lambda p, q: any(abs(p[i] - q[i]) > CONTRAST_PER_CHN for i in range(3))
    width, height = len(img[0]), len(img)
    discont = []
    nh, nv = 0, 0
    for i in range(height):
        discont.append([])
        for j in range(width):
            h = check(img[i][j], img[i+1][j]) if i < height - 1 else check(img[i][j], [0, 0, 0])
            v = check(img[i][j], img[i][j+1]) if j < width - 1 else check(img[i][j], [0, 0, 0])
            if h: nh += 1
            if v: nv += 1
            discont[i].append((h, v))
    print("%s horizontal found" % nh)
    print("%s vertical found" % nv)

    #step 2 - identify shapes
    #U - Orthogonal lines are on same side
    #Z - Two orthogonal lines cross both half-planes formed by the current separation line
    #L - Separation line at image border

    #step 3 - blend


#http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
#https://github.com/NVIDIAGameWorks/GraphicsSamples/blob/master/samples/es3-kepler/FXAA/FXAA3_11.h
def fxaa(img):
    width, height = len(img[0]), len(img)
    FXAA_EDGE_THRESHOLD = 1/8
    FXAA_EDGE_THRESHOLD_MIN = 1/16
    FXAA_SUBPIX_TRIM = 1/4
    FXAA_SUBPIX_TRIM_SCALE = 1 #on/off?
    FXAA_SUBPIX_CAP = 2/4
    FXAA_SEARCH_STEPS = 5 #is this good?
    FXAA_SEARCH_ACCELERATION = 1
    FXAA_SEARCH_THRESHOLD = 1/4
    def luma(rgb): return rgb[1] * (0.587/0.299) + rgb[0]
    def flot(rgb): return list(map(lambda c : c / 255, rgb))
    def byte(rgb): return list(map(lambda c : int(c * 255), rgb))
    def fxaa_shader(x, y):
        n = flot(img[y-1][x]) if y - 1 > 0 else img[y][x]
        e = flot(img[y][x+1]) if x + 1 < width else img[y][x]
        s = flot(img[y+1][x]) if y + 1 < height else img[y][x]
        w = flot(img[y][x-1]) if x - 1 > 0 else img[y][x]
        m = flot(img[y][x])
        ln, le, ls, lw, lm = luma(n), luma(e), luma(s), luma(w), luma(m)
        
        luma_min = min(nw, ne, sw, se, m)
        luma_max = max(nw, ne, sw, se, m)
        rng = luma_max - luma_min
        # if we are within some threshold we will early exit, it is too dark to notice anti aliasing anyway
        if rng < max(FXAA_EDGE_THRESHOLD_MIN, luma_max * FXAA_EDGE_THRESHOLD):
            return img[y][x]
        #contast is estimated as the absolute difference in luma from the average of neighbours
        luma_av = (n + e + s + w)/4
        rng_av = luma_av - m
        blend = max(0, (rng_av / rng) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE
        blend = min(FXAA_SUBPIX_CAP, blend)
        nw = flot(img[y-1][x-1]) if y - 1 > 0      and x - 1 > 0     else img[y][x]
        ne = flot(img[y-1][x+1]) if y - 1 > 0      and x + 1 < width else img[y][x]
        se = flot(img[y+1][x+1]) if y + 1 < height and x + 1 < width else img[y][x]
        sw = flot(img[y+1][x-1]) if y + 1 < height and x - 1 > 0     else img[y][x]
        lnw, lne, lse, lsw= luma(nw), luma(ne), luma(se), luma(sw)
        low_rgb = list(map(lambda x : x/9, map(sum, zip(n, e, s, w, ne, nw, se, sw, m))))
        edge_vert = (abs(0.25 * lnw - 0.5 * ln + 0.25 * lne) +
                     abs(0.50 * lw  - 1.0 * lm + 0.50 * le ) +
                     abs(0.25 * lsw - 0.5 * ls + 0.25 * lse))
        edge_horz = (abs(0.25 * lnw - 0.5 * lw + 0.25 * lsw) +
                     abs(0.50 * ln  - 1.0 * lm + 0.50 * ls ) +
                     abs(0.25 * lne - 0.5 * le + 0.25 * lse))
        horz_span = edge_horz >= edge_vert
        
    for y in range(height):
        for x in range(width):
            img[y][x] = fxaa_shader(x, y)
            print(img[y][x])

def dot(u, v):
    return u[0]*v[0] + u[1]*v[1] + u[2]*v[2]

from math import floor

def fxaa2(img):
    width, height = len(img[0]), len(img)
    reducemul = 1/8
    reducemin = 1/128
    u_strength =20.0
    u_texel = (1/width, 1/height)
    def texture_2d(img, x, y):
        if x < 0.0: x = 0.0
        if y < 0.0: y = 0.0
        if x > 1.0: x = 1.0
        if y > 1.0: y = 1.0
        try:
            return img[int(round(y*(height-1)))][int(round(x*(width-1)))]
        except IndexError:
            print(x, y)
    def rgb_percent(rgb):
        r, g, b = rgb
        return (r/255, g/255, b/255)
    def rgb_int(rgb):
        r, g, b = rgb
        return (int(r*255), int(g*255), int(b*255))
    def fxaa_shader(x_, y_):
        (x, y) = (x_/width, y_/height)        
        centre = texture_2d(img, x, y)
        nw = rgb_percent(texture_2d(img, x-u_texel[0], y-u_texel[1]))
        ne = rgb_percent(texture_2d(img, x+u_texel[0], y-u_texel[1]))
        sw = rgb_percent(texture_2d(img, x-u_texel[0], y+u_texel[1]))
        se = rgb_percent(texture_2d(img, x+u_texel[0], y+u_texel[1]))
        gray = (0.299, 0.587, 0.114)
        mono_centre = dot(centre, gray)
        mono_nw = dot(nw, gray)
        mono_ne = dot(ne, gray)
        mono_sw = dot(sw, gray)
        mono_se = dot(se, gray)

        mono_min = min(mono_centre, mono_nw, mono_ne, mono_sw, mono_se)
        mono_max = max(mono_centre, mono_nw, mono_ne, mono_sw, mono_se)

        dir_vec = (-((mono_nw + mono_ne) - (mono_sw + mono_se)), ((mono_nw + mono_sw) - (mono_ne + mono_se)))
        dir_reduce = max((mono_nw + mono_ne + mono_sw + mono_se) * reducemul * 0.25, reducemin)
        dir_min = 1.0 / (min(abs(dir_vec[0]), abs(dir_vec[1])) + dir_reduce)
        dir_vec = (min(u_strength, max(-u_strength, dir_vec[0] * dir_min))*u_texel[0], min(u_strength, max(-u_strength, dir_vec[1] * dir_min))*u_texel[1])

        result_a = tuple(map(lambda x: x[0] * x[1], zip((0.5, 0.5, 0.5), map(lambda x: x[0] + x[1], zip(rgb_percent(texture_2d(img, x + dir_vec[0] * -0.166667, y + dir_vec[1] * -0.166667)), rgb_percent(texture_2d(img, x + dir_vec[0] * 0.166667, y + dir_vec[1] * 0.166667)))))))
        a_x, a_y, a_z = result_a
        #print(a_x, a_y, a_z)
        b_x, b_y, b_z = map(lambda x: x[0] + x[1], zip(rgb_percent(texture_2d(img, x + dir_vec[0] * -0.5, y + dir_vec[1] * -0.5)), rgb_percent(texture_2d(img, x + dir_vec[0] * 0.5, y + dir_vec[1] * 0.5))))
        result_b = (a_x * 0.5 + 0.25 * b_x, a_y * 0.5 + 0.25 * b_y, a_z * 0.5 + 0.25 * b_z)
        mono_b = dot(result_b, gray)

        #print(result_a)
        #print(result_b)
        colour = result_a if mono_b < mono_min or mono_b > mono_max else result_b
        return rgb_int(colour)

    img_ = []
    for y in range(height):
        img_.append([])
        for x in range(width):
            img_[y].append(fxaa_shader(x, y))

    return img_
            
def sraa(img):
    pass

img = load_ppm("sample.ppm")

img = fxaa2(img)

save_ppm("sampleaa.ppm", img)
