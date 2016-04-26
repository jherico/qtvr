import QtQuick 2.5

QtObject {
    id: shadertoyAPI
    objectName: "shadertoyAPI"
    // API docs: https://www.shadertoy.com/api
    readonly property string apiKey: "Nt8tw7"
    readonly property string apiBaseUrl: "https://www.shadertoy.com/api/v1";

    function fetchShaderList(callback) {
        request("/shaders", {}, function(results) {
            callback(JSON.parse(results).Results);
        });
    }

    function fetchShader(shaderId, callback) {
        request("/shaders/" + shaderId, {}, function(shaderJson) {
            var shader = JSON.parse(shaderJson).Shader;
            callback(shader);
        });
    }

    // Sort
    // Query shaders sorted by "name", "love", "popular", "newest", "hot" (by default, it uses "popular").
    // https://www.shadertoy.com/api/v1/shaders/query/string?sort=newest&key=appkey

    // Pagination
    // https://www.shadertoy.com/api/v1/shaders/query/string?from=5&num=25&key=appkey

    // Filter
    // Query shaders with filters: "vr", "soundoutput", "soundinput", "webcam", "multipass", "musicstream" (by default, there is no filter)
    // https://www.shadertoy.com/api/v1/shaders/query/string?filter=vr&key=appkey
    function queryShaders(query, params, callback) {
        if (!callback) {
            callback = params;
            params = {}
        }

        request("/shaders/query" + (query ? "/" + query : ""), callback ? params : {}, function(results) {
            var shaderList = JSON.parse(results).Results;
            callback(shaderList);
        });
    }

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
        console.log("Requesting " + url)
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                if (xhr.status === 200) {
                    callback(xhr.responseText);
                } else {
                    console.warn("Status result " + xhr.status)
                }
            }
        };
        xhr.open('GET', url, true);
        xhr.send('');
    }
}
