import QtQuick 2.5

QtObject {
    id: shadertoyAPI
    objectName: "shadertoyAPI"
    // API docs: https://www.shadertoy.com/api
    property string apiKey: "Nt8tw7"

    property bool offline: true

    signal receivedShaderList(string shaderList)
    signal receivedShader(string shaderList)

    // FIXME persist to disk between sessions?
    property var cache: { "__placeholder": null }

    function fetchShaderList(callback) {
        if (offline) {
            callback(shadertoyCache.fetchShaderList());
            return;
        }

        request("/shaders", {}, function(results) {
            receivedShaderList(results);
            callback(JSON.parse(results).Results);
        })
    }


    function fetchShaderInfo(shaderId, callback, ignoreCache) {
        if (offline) {
            callback(shadertoyCache.fetchShaderInfo(shaderId));
            return;
        }

        fetchShader(shaderId, function(result) {
            callback(result.info)
        }, ignoreCache);
    }

    function fetchShader(shaderId, callback, ignoreCache) {
        if (offline) {
            console.log("Requested shader " + shaderId);
            var result = shadertoyCache.fetchShader(shaderId);
            console.log("Result " + result)
            callback(result);
            return;
        }

        if (cache[shaderId] && !ignoreCache) {
            console.log("Got cached data for " + shaderId);
            callback(cache[shaderId]);
            return;
        }

        request("/shaders/" + shaderId, {}, function(shaderInfo) {
            console.log("Got network data for " + shaderId);
            receivedShader(shaderInfo);
            shaderInfo = JSON.parse(shaderInfo)
            cache[shaderId] = shaderInfo.Shader;
            callback(shaderInfo.Shader);
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
    function queryShaders(query, params, callback) {
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
            var shaderList = JSON.parse(results).Results;
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
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
                callback(xhr.responseText);
            }
        };
        xhr.open('GET', url, true);
        xhr.send('');
    }
}
