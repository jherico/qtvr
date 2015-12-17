//  lightSaberEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 12/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script creates a lightsaber.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../../libraries/utils.js");
    var _this;
    // this is the "constructor" for the entity as a JS object we don't do much here
    var LightSaber = function() {
        _this = this;
        this.colorPalette = [{
            red: 0,
            green: 200,
            blue: 40
        }, {
            red: 200,
            green: 10,
            blue: 40
        }];
    };

    LightSaber.prototype = {
        isGrabbed: false,

        startNearGrab: function() {
            Entities.editEntity(this.beam, {
                isEmitting: true
            });
        },

        continueNearGrab: function() {

        },

        releaseGrab: function() {
            Entities.editEntity(this.beam, {
                isEmitting: false
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.createBeam();
        },

        unload: function() {
            Entities.deleteEntity(this.beam);
            // Entities.deleteEntity(this.beamTrail);
        },

        createBeam: function() {


            this.props = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            var forwardVec = Quat.getFront(Quat.multiply(this.props.rotation, Quat.fromPitchYawRollDegrees(-90, 0, 0)));
            // forwardVec = Vec3.normalize(forwardVec);
            var forwardQuat = orientationOf(forwardVec);
            var position = Vec3.sum(this.props.position, Vec3.multiply(Quat.getFront(this.props.rotation), 0.1));
            position.z += 0.1;
            position.x += -0.035;
            var color = this.colorPalette[randInt(0, this.colorPalette.length)];
            var props = {
                type: "ParticleEffect",
                position: position,
                parentID: this.entityID,
                isEmitting: false,
                "colorStart": color,
                color: {
                    red: 200,
                    green: 200,
                    blue: 255
                },
                "colorFinish": color,
                "maxParticles": 100000,
                "lifespan": 2,
                "emitRate": 1000,
                emitOrientation: forwardQuat,
                "emitSpeed": .4,
                "speedSpread": 0.0,
                "emitDimensions": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "polarStart": 0,
                "polarFinish": .0,
                "azimuthStart": .1,
                "azimuthFinish": .01,
                "emitAcceleration": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "accelerationSpread": {
                    "x": .00,
                    "y": .00,
                    "z": .00
                },
                "radiusStart": 0.03,
                radiusFinish: 0.025,
                "alpha": 0.7,
                "alphaSpread": .1,
                "alphaStart": 0.5,
                "alphaFinish": 0.5,
                // "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                "textures": "file:///C:/Users/Eric/Desktop/beamParticle.png?v1" + Math.random(),
                emitterShouldTrail: false
            }
            this.beam = Entities.addEntity(props);

            // props.emitterShouldTrail = true;
            // this.beamTrail = Entities.addEntity(props);

        }
    };
    // entity scripts always need to return a newly constructed object of our type
    return new LightSaber();
});