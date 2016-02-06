import QtQuick 2.5

QtObject {
    id: shadertoyAPI
    objectName: "shadertoyAPI"
    // API docs: https://www.shadertoy.com/api
    property string apiKey: "Nt8tw7"

    property bool offline: false

    function fetchShaderInfo(shaderId, callback) {
        fetchShader(shaderId, function(result) { callback(result.info) });
    }

    function fetchShaderList(callback) {
        if (offline) {
            callback(shadertoyCache.getShaderList());
            return;
        }

        console.debug("Fetching all shader IDs");
        request("/shaders", {}, function(results) {
            shadertoyCache.setShaderList(results);
            callback(JSON.parse(results).Results);
        })
    }

    function fetchShader(shaderId, callback, ignoreCache) {
        console.debug("Fetch shader called with ")
        if (!offline && (ignoreCache || !shadertoyCache.hasShader(shaderId))) {
            callback(shadertoyCache.getShader(shaderId));
            return;
        }
        console.debug("Shader not found in cache, fetching " + shaderId);
        request("/shaders/" + shaderId, {}, function(shaderJson) {
            console.log("Got API response for shader " + shaderId);
            var shader = shadertoyCache.setShader(shaderId, shaderJson);
            console.log("Parsed shader is " + shader);
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
        console.log("Calling shader query " + params)
        if (!callback) {
            callback = params;
            params = {}
        }
        if (offline) {
            console.log(shadertoyCache)
            callback(shadertoyCache.queryShaders(query, params));
            return;
        }

        request("/shaders/query" + (query ? "/" + query : ""), callback ? params : {}, function(results) {
            console.debug("Processing query results");
            var shaderList = JSON.parse(results).Results;
            console.log(shaderList);
            (callback ? callback : params)(shaderList);
        });
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
        console.log("Requesting " + url)
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                if (xhr.status === 200) {
                    callback(xhr.responseText);
                } else {
                    console.warn("Status result " + xhr.status)
                }
            } else {
                console.log(xhr.readyState);
            }
        };
        xhr.open('GET', url, true);
        xhr.send('');

    }
}
