import QtQuick 2.5

QtObject {
    id: shadertoy

    property var activeTextures: [ misc[0], misc[0], misc[0], misc[0] ]


    readonly property var textures: [
        {
            preview: "/presets/previz/tex00.jpg",
            type: 'texture',
            id: 1,
            source:'/presets/tex00.jpg',
            sampler: { filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte', },
            tip: "<b>Resolution:</b> 512 x 512<br><b>Format:</b> rgb<br><br><b>Source:</b> unknown"
        },
        { preview: "/presets/previz/tex01.jpg", type: 'texture', id:2,  source:'/presets/tex01.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 1024 x 1024<br><b>Format:</b> rgb<br><br><b>Source:</b> unkown" },
        { preview: "/presets/previz/tex02.jpg", type: 'texture', id:3,  source:'/presets/tex02.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 512 x 512<br><b>Format:</b> rgb<br><br><b>Source:</b> unkown" },
        { preview: "/presets/previz/tex03.jpg", type: 'texture', id:4,  source:'/presets/tex03.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 512 x 512<br><b>Format:</b> rgb<br><br><b>Source:</b> unkown" },
        { preview: "/presets/previz/tex04.jpg", type: 'texture', id:5,  source:'/presets/tex04.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 512 x 512<br><b>Format:</b> rgb<br><br><b>Source:</b> unkown" },
        { preview: "/presets/previz/tex05.jpg", type: 'texture', id:6,  source:'/presets/tex05.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 1024 x 1024<br><b>Format:</b> rgb<br><br><b>Source:</b> unkown" },
        { preview: "/presets/previz/tex06.jpg", type: 'texture', id:7,  source:'/presets/tex06.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 1024 x 1024<br><b>Format:</b> rgb<br><br><b>Source:</b> unkown" },
        { preview: "/presets/previz/tex07.jpg", type: 'texture', id:8,  source:'/presets/tex07.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 1024 x 1024<br><b>Format:</b> rgb<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex17.jpg", type: 'texture', id:45, source:'/presets/tex17.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 1024 x 1024<br><b>Format:</b> rgb<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex18.jpg", type: 'texture', id:46, source:'/presets/tex18.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 1024 x 1024<br><b>Format:</b> rgb<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex19.jpg", type: 'texture', id:47, source:'/presets/tex19.png', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 512 x 512<br><b>Format:</b> r<br><br><b>Source:</b> http://photosculpt.net/gallery/textures-seamless-tileable/" },
        { preview: "/presets/previz/tex20.jpg", type: 'texture', id:48, source:'/presets/tex20.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 1024 x 1024<br><b>Format:</b> rgb<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex08.jpg", type: 'texture', id:9,  source:'/presets/tex08.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 512 x 512<br><b>Format:</b> rgb<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex09.jpg", type: 'texture', id:10, source:'/presets/tex09.jpg', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 1024 x 1024<br><b>Format:</b> rgb<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex10.png", type: 'texture', id:15, source:'/presets/tex10.png', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 64 x 64<br><b>Format:</b> r<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex11.png", type: 'texture', id:16, source:'/presets/tex11.png', sampler:{ filter: 'mipmap', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 64 x 64<br><b>Format:</b> rgba<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex10.png", type: 'texture', id:17, source:'/presets/tex12.png', sampler:{ filter: 'mipmap',  wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 256 x 256<br><b>Format:</b> r<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex15.png", type: 'texture', id:28, source:'/presets/tex15.png', sampler:{ filter: 'nearest', wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 8 x 8<br><b>Format:</b> r<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex16.png", type: 'texture', id:30, source:'/presets/tex16.png', sampler:{ filter: 'mipmap',  wrap: 'repeat', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 256 x 256<br><b>Format:</b> rgba<br><br><b>Source:</b> www.shadertoy.com" },
        { preview: "/presets/previz/tex14.png", type: 'texture', id:14, source:'/presets/tex14.png', sampler:{ filter: 'nearest', wrap: 'clamp',  vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 256 x 32<br><b>Format:</b> rgb<br><br><b>Source:</b> www.shadertoy.com" }
    ]

    readonly property var cubemaps:[
        { preview: "/presets/previz/cube00.png", type:'cubemap', id:22, source:'/presets/cube00_0.jpg', sampler:{ filter: 'mipmap',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 512x512<br><br><b>Source:</b> http://www.pauldebevec.com/Probes" },
        { preview: "/presets/previz/cube01.png", type:'cubemap', id:23, source:'/presets/cube01_0.png', sampler:{ filter: 'mipmap',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 64x64<br><br><b>Source:</b> http://www.pauldebevec.com/Probes" },
        { preview: "/presets/previz/cube02.png", type:'cubemap', id:24, source:'/presets/cube02_0.jpg', sampler:{ filter: 'mipmap',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 256x256<br><br><b>Source:</b> http://www.pauldebevec.com/Probes" },
        { preview: "/presets/previz/cube03.png", type:'cubemap', id:25, source:'/presets/cube03_0.png', sampler:{ filter: 'mipmap',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 64x64<br><br><b>Source:</b> http://www.pauldebevec.com/Probes" },
        { preview: "/presets/previz/cube04.png", type:'cubemap', id:26, source:'/presets/cube04_0.png', sampler:{ filter: 'mipmap',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 128x128<br><br><b>Source:</b> http://www.pauldebevec.com/Probes" },
        { preview: "/presets/previz/cube05.png", type:'cubemap', id:27, source:'/presets/cube05_0.png', sampler:{ filter: 'mipmap',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Resolution:</b> 64x64<br><br><b>Source:</b> http://www.pauldebevec.com/Probes" }
    ]

    readonly property var misc: [
        { preview: "/presets/previz/none.png", type: null, id:-1, source:null},
        { preview: "/presets/previz/keyboard.png", type: 'keyboard', id:33, source:null, sampler:{ filter: 'linear', wrap: 'clamp', vflip:true, srgb:false, internal:'byte' } },
        { preview: "/presets/previz/webcam.png", type: 'webcam', id:31, source:'/presets/webcam.png', sampler:{ filter: 'linear', wrap: 'clamp', vflip:true, srgb:false, internal:'byte' } },
        { preview: "/presets/previz/mic.png", type: 'mic', id:32, source:'/presets/mic.png', sampler:{ filter: 'linear', wrap: 'clamp', vflip:true, srgb:false, internal:'byte' } },
        { preview: "/presets/previz/soundcloud.png", type: 'mic', id:32, source:'/presets/mic.png', sampler:{ filter: 'linear', wrap: 'clamp', vflip:true, srgb:false, internal:'byte' } }
    ]

    readonly property var buffers: [
        { preview: "/presets/previz/buffer00.png", type: 'buffer', id:257,  source:'/presets/previz/buffer00.png', sampler:{ filter: 'linear', wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "Render buffer A" },
        { preview: "/presets/previz/buffer01.png", type: 'buffer', id:258,  source:'/presets/previz/buffer01.png', sampler:{ filter: 'linear', wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "Render buffer B" },
        { preview: "/presets/previz/buffer02.png", type: 'buffer', id:259,  source:'/presets/previz/buffer02.png', sampler:{ filter: 'linear', wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "Render buffer C" },
        { preview: "/presets/previz/buffer03.png", type: 'buffer', id:260,  source:'/presets/previz/buffer03.png', sampler:{ filter: 'linear', wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "Render buffer D" }
    ]

    readonly property var videos: [
        { preview: "/presets/previz/vid00.gif", type: 'video', id:11, source:'/presets/vid00.ogv',  sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Duration:</b> 34s<br><br><b>Source:</b> www.google.com" },
        { preview: "/presets/previz/vid01.gif", type: 'video', id:12, source:'/presets/vid01.webm', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Duration:</b> 3m 40s<br><br><b>Source:</b> http://www.youtube.com/watch?v=I02Ss2VUM3U" },
        { preview: "/presets/previz/vid02.gif", type: 'video', id:29, source:'/presets/vid02.ogv',  sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Duration:</b> 29s<br><br><b>Source:</b> https://archive.org/details/movies" },
        { preview: "/presets/previz/vid03.gif", type: 'video', id:36, source:'/presets/vid03.webm', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<b>Duration:</b> 2m 19s<br><br><b>Source:</b> https://www.youtube.com/watch?v=T2k0PZCPQMk" }
    ]

    readonly property var music: [
        { preview: "/presets/previz/mzk00.png", type: 'music', id:13, source:'/presets/mzk00.mp3', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<br>Electronebulae<span><b>Title:</b> Electronebulae<br><b>Duration:</b> 4m 18s<br><b>Author:</b> stage7" },
        { preview: "/presets/previz/mzk00.png", type: 'music', id:18, source:'/presets/mzk01.mp3', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<br>Experiment<span><b>Title:</b> Experiment<br><b>Duration:</b> 3m 48s<br><b>Author:</b> iq" },
        { preview: "/presets/previz/mzk00.png", type: 'music', id:19, source:'/presets/mzk02.mp3', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<br>8 bit mentality<span><b>Title:</b> 8 bit mentalit<br><b>Duration:</b> 3m 25s<br><b>Author:</b> stage7" },
        { preview: "/presets/previz/mzk00.png", type: 'music', id:20, source:'/presets/mzk03.mp3', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<br>X'TrackTure<span><b>Title:</b> X'TrackTure<br><b>Duration:</b> 3m 50s<br><b>Author:</b> josSs" },
        { preview: "/presets/previz/mzk00.png", type: 'music', id:21, source:'/presets/mzk04.mp3', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<br>ourpithyator<span><b>Title:</b> ourpithyator<br><b>Duration:</b> 7m 32s<br><b>Author:</b> AudeoFlow &amp; Gizma" },
        { preview: "/presets/previz/mzk00.png", type: 'music', id:34, source:'/presets/mzk05.mp3', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<br>Tropical Beeper<span><b>Title:</b> Tropical Beepe<br><b>Duration:</b> 2m 42s<br><b>Author:</b> Dave Hoskins" },
        { preview: "/presets/previz/mzk00.png", type: 'music', id:35, source:'/presets/mzk06.mp3', sampler:{ filter: 'linear',  wrap: 'clamp', vflip:true, srgb:false, internal:'byte' }, tip: "<br>Most Geometric Person<span><b>Title:</b> Most Geometric Person<br><b>Duration:</b> 1m 15s<br><b>Author:</b> Noby" },
    ]

    readonly property var inputs: [
        { name: "Misc", textures: misc },
        { name: "Buffers", textures: buffers },
        { name: "Textures", textures: textures },
        { name: "Videos", textures: videos },
        { name: "Cubemaps", textures: cubemaps },
        { name: "Music", textures: music },
    ]

    property var shadersInfoCache: {
        "ZZZZZZ": {}
    }

    readonly property var sortFields: [
        "popular", "name", "love", "newest", "hot"
    ]

    readonly property var filterFields: [
        "none", "vr", "soundoutput", "soundinput", "webcam", "multipass", "musicstream"
    ]

    // API docs: https://www.shadertoy.com/api
    property string apiKey: "Nt8tw7"

    function fetchShaderList(callback) {
        request("/shaders", {}, callback)
    }

    function fetchShader(shaderId, callback) {
        if (shadersInfoCache[shaderId]) {
            console.log("Got cached data for " + shaderId);
            callback(shadersInfoCache[shaderId]);
            return;
        }

        request("/shaders/" + shaderId, {}, function(shaderInfo) {
            console.log("Got network data for " + shaderId);
            shadersInfoCache[shaderId] = shaderInfo;
            callback(shaderInfo);
        })
    }

    // Sort
    // Query shaders sorted by "name", "love", "popular", "newest", "hot" (by default, it uses "popular").
    // https://www.shadertoy.com/api/v1/shaders/query/string?sort=newest&key=appkey

    // Pagination
    // https://www.shadertoy.com/api/v1/shaders/query/string?from=5&num=25&key=appkey

    // Filter
    // Query shaders with filters: "vr", "soundoutput", "soundinput", "webcam", "multipass", "musicstream" (by default, there is no filter)
    // https://www.shadertoy.com/api/v1/shaders/query/string?filter=vr&key=appkey
    function searchShaders(query, params, callback) {
        request("/shaders/query" + (query ? "/" + query : ""), callback ? params : {}, callback ? callback : params);
    }

    readonly property string apiBaseUrl: "https://www.shadertoy.com/api/v1";

    function request(path, params, callback) {
        var xhr = new XMLHttpRequest();
        var queryString = "key=" + apiKey;
        var queryParamNames = Object.keys(params);
        for (var i = 0; i < queryParamNames.length; ++i) {
            var name = queryParamNames[i];
            if (name === "filter" && params[name] === "none") {
                continue;
            }
            queryString += "&" + name + "=" + params[name];
        }

        var url = apiBaseUrl + path + "?" + queryString;
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
//                console.log(url)
//                console.log(xhr.responseText)
                callback(JSON.parse(xhr.responseText));
            }
        };
        xhr.open('GET', url, true);
        xhr.send('');
    }
}

/*


where string is your search string such as tags, usernames, words...


Get a shader from a shader ID.

https://www.shadertoy.com/api/v1/shaders/shaderID?key=appkey

where shaderID is the same ID used in the Shadertoy URLs, and also the values returned by the "Query Shaders".


Access the assets.

When you retrieve a shader you will see a key called "inputs", this can be a texture/video/keyboard/sound used by the shader. The JSON returned when accessing a shader will look like this:

[..]{"inputs":[{"id":17,"src":"/presets/tex12.png","ctype":"texture","channel":0}[..]

To access this specific asset you can just cut and paste this path https://www.shadertoy.com/presets/tex12.png


Get all shaders.

https://www.shadertoy.com/api/v1/shaders?key=appkey
  */
