/* Copyright STIFTELSEN SINTEF 2014
 *
 * This file is part of the Tinia Framework.
 *
 * The Tinia Framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The Tinia Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Tinia Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

dojo.require("gui.ProxyModel");

dojo.declare("gui.ProxyRenderer", null, {


    constructor: function(glContext, exposedModel, viewerKey) {

        // Number of proxy geometry splats (gl Points) in each direction, covering the viewport.
        // (Note that as long as glPoints are square, the ratio between these numbers should ideally equal the aspect ratio of the viewport.)
        this._splats_x = 32;
        this._splats_y = 32;

        // This factor is just a guestimate at how much overlap we need between splats for those being moved toward the observer to fill in
        // gaps due to expansion caused by the perspective view, before new depth buffers arrive.
        this._splatOverlap = 1.0; // 1.0) splats are "shoulder to shoulder", 2.0) edge of one circular splat passes through center of neighbour to side or above/below

        this._depthRingSize = 2;

        // ---------------- End of configuration section -----------------

        this._depthBufferCounter = 0;
        this._subscriptionCounter = 0;
        this.gl = glContext;
        this._depthRingCursor = 0;

        dojo.xhrGet( { url: "gui/autoProxy.fs",
                        handleAs: "text",
                        load: dojo.hitch(this, function(data, ioArgs) {
                            this._splat_fs_src = data;
                        })
                    } );
        dojo.xhrGet( { url: "gui/autoProxy.vs",
                        handleAs: "text",
                        load: dojo.hitch(this, function(data, ioArgs) {
                            this._splat_vs_src = data;
                        })
                    } );

        dojo.subscribe("/model/updateSendStart", dojo.hitch(this, function(xml) {
            this._subscriptionCounter++;
            console.log("Subscriber: Setting matrices, _subscriptionCounter: " + this._subscriptionCounter);
            var viewer = exposedModel.getElementValue(viewerKey);
            // These are matrices available when we set the depth texture. But are these the exact
            // matrices used by the server to produce the depth image? Or do they just coincide
            // more or less in time? This was the first attempt. It does not look quite right, but what is really the problem?
            // It becomes much better when we use the matrices passed along with the depth buffer itself, see method setViewMat below.
            this._proxyModelRing[this._depthRingCursor].projection         = viewer.getElementValue("projection");
            this._proxyModelRing[this._depthRingCursor].projection_inverse = mat4.inverse(mat4.create(viewer.getElementValue("projection")));
            this._proxyModelRing[this._depthRingCursor].to_world           = mat4.inverse(mat4.create(viewer.getElementValue("modelview")));
            this._proxyModelRing[this._depthRingCursor].from_world         = viewer.getElementValue("modelview");
        }));

        this._proxyModelRing = new Array(this._depthRingSize);
        for (i=0; i<this._depthRingSize; i++) {
            this._proxyModelRing[i] = new gui.ProxyModel(this.gl);
        }

        this._splatVertexBuffer = this.gl.createBuffer();
        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this._splatVertexBuffer);
        this._splatCoordinates = new Float32Array( this._splats_x*this._splats_y*2 );
        for (i=0; i<this._splats_y; i++) {
            for (j=0; j<this._splats_x; j++) {
                var u = (j+0.5)/this._splats_x;
                var v = (i+0.5)/this._splats_y;
                this._splatCoordinates[(this._splats_x*i+j)*2     ] = -1.0*(1.0-u) + 1.0*u;
                this._splatCoordinates[(this._splats_x*i+j)*2 + 1 ] = -1.0*(1.0-v) + 1.0*v;
            }
        }
        this.gl.bufferData(this.gl.ARRAY_BUFFER, new Float32Array(this._splatCoordinates), this.gl.STATIC_DRAW);

        console.log("Constructor ended");
    },


    compileShaders: function() {
        console.log("Shader source should now have been read from files, compiling and linking program...");

        var splat_fs = this.gl.createShader(this.gl.FRAGMENT_SHADER);
        this.gl.shaderSource(splat_fs, this._splat_fs_src);
        this.gl.compileShader(splat_fs);
        if (!this.gl.getShaderParameter(splat_fs, this.gl.COMPILE_STATUS)) {
            alert("An error occurred compiling the splat_fs: " + this.gl.getShaderInfoLog(splat_fs));
            return null;
        }

        var splat_vs = this.gl.createShader(this.gl.VERTEX_SHADER);
        this.gl.shaderSource(splat_vs, this._splat_vs_src);
        this.gl.compileShader(splat_vs);
        if (!this.gl.getShaderParameter(splat_vs, this.gl.COMPILE_STATUS)) {
            alert("An error occurred compiling the splat_vs: " + this.gl.getShaderInfoLog(splat_vs));
            return null;
        }

        this._splatProgram = this.gl.createProgram();
        this.gl.attachShader(this._splatProgram, splat_vs);
        this.gl.attachShader(this._splatProgram, splat_fs);
        this.gl.linkProgram(this._splatProgram);
        if (!this.gl.getProgramParameter(this._splatProgram, this.gl.LINK_STATUS)) {
            alert("Unable to initialize the shader program. (gl.LINK_STATUS not ok,)");
        }
    },


    setDepthData: function(imageAsText, depthBufferAsText, viewMatAsText, projMatAsText) {
        this._depthBufferCounter++;
        console.log("setDepthBuffer: Setting buffer, count = " + this._depthBufferCounter);
        this._proxyModelRing[this._depthRingCursor].setNotReady();
        this._proxyModelRing[this._depthRingCursor].setDepthBuffer(depthBufferAsText);
        this._proxyModelRing[this._depthRingCursor].setRGBimage(imageAsText);
        this._proxyModelRing[this._depthRingCursor].setMatrices(viewMatAsText, projMatAsText);
        // console.log("Depth buffer set");
     },


    render: function(matrices) {

        if ( (!this._splatProgram) && (this._splat_vs_src) && (this._splat_fs_src) ) {
            this.compileShaders();
        }

        if ( (this._proxyModelRing[this._depthRingCursor].isReady()) && (this._splatProgram) ) {

            this.gl.clearColor(0.2, 0.2, 0.2, 1.0);
            this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);

            // Strange... Blending disabled. Clearing done before proxy rendering. Then, still, ...:
            //
            // (Two different effects are observed with some settings: 1. snapshot is lingering behind proxy even when glClear is done,
            // and 2. snapshot residuals are observed in cleared parts and redrawn (by proxy) parts.)
            //
            // clearColor A     Proxy rendered A    Result
            // ------------------------------------------------------------------------------------
            // 0                0                   snapShot lingers in un-re-rendered parts. (Seems less saturated, as if scaled with 0.5 perhaps)
            //                                      New proxy seems to be blended with old image, get saturation in some places.
            //
            // 0                0.5                 snapShot lingers in un-re-rendered parts.
            //                                      Also lingers in re-rendered parts! As if blending was enabled.
            //                                      In other words, both "effects" at the same time.
            //
            // 0                1                   snapShot lingers in un-re-rendered (by proxy geometry) parts of the image.
            //                                      But the background color specified is used around the snapShot fragments actually set, even though the
            //                                      snapShot itself should have another background (black)!
            //
            // 1                0                   snapShot does not linger in un-re-rendered parts, here the clear-color is used
            //                                      But; lingers in re-rendered parts! As if blending was enabled.
            //                                      But if any sort of blending is being done, why does not the snapShot linger in un-re-rendered parts?!
            //                                      (Why does the clear remove old snapshot from un-rendered parts but not those that are overdrawn with new proxy fragments?!)
            //
            // 1                0.5                 Sames as for proxy-alpha=1, but we get saturation in lesser extent (snapshot + proxy < 1 e.g. for snap-red + proxy-cyan)
            //                                                                                                                                      (1, 0, 0)  + 0.5*(0, 1, 1) = (1, 0.5, 0.5)
            //                                                                                                                      instead of      (1, 0, 0)  + 1.0*(0, 1, 1) = (1, 1, 1 ) it looks like...
            //
            // 1                1                   More in line with expectations: no lingering snapShot at all. Background cleared to selected color everywhere.


            //            this.gl.enable(this.gl.BLEND);
            this.gl.disable(this.gl.BLEND);

//            this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.SRC_ALPHA); // s-factor, d-factor
//            this.gl.disable(this.gl.DEPTH_TEST);


            this.gl.useProgram(this._splatProgram);
            var vertexPositionAttribute = this.gl.getAttribLocation( this._splatProgram, "aVertexPosition" );
            this.gl.enableVertexAttribArray( vertexPositionAttribute );

            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this._splatVertexBuffer);

            this.gl.activeTexture(this.gl.TEXTURE0);
            this.gl.bindTexture(this.gl.TEXTURE_2D, this._proxyModelRing[this._depthRingCursor].depthTexture);
            this.gl.uniform1i( this.gl.getUniformLocation(this._splatProgram, "uSampler"), 0 );

            this.gl.activeTexture(this.gl.TEXTURE1);
            this.gl.bindTexture(this.gl.TEXTURE_2D, this._proxyModelRing[this._depthRingCursor].rgbTexture);
            this.gl.uniform1i( this.gl.getUniformLocation(this._splatProgram, "rgbImage"), 1 );

            if (this.gl.getUniformLocation(this._splatProgram, "MV")) {
                this.gl.uniformMatrix4fv( this.gl.getUniformLocation(this._splatProgram, "MV"), false, matrices.m_from_world );
            }
            if (this.gl.getUniformLocation(this._splatProgram, "PM")) {
                this.gl.uniformMatrix4fv( this.gl.getUniformLocation(this._splatProgram, "PM"), false, matrices.m_projection );
            }
            if (this.gl.getUniformLocation(this._splatProgram, "depthPMinv")) {
                this.gl.uniformMatrix4fv( this.gl.getUniformLocation(this._splatProgram, "depthPMinv"), false, this._proxyModelRing[this._depthRingCursor].projection_inverse );
            }
            if (this.gl.getUniformLocation(this._splatProgram, "depthMVinv")) {
                this.gl.uniformMatrix4fv( this.gl.getUniformLocation(this._splatProgram, "depthMVinv"), false, this._proxyModelRing[this._depthRingCursor].to_world );
            }
            if (this.gl.getUniformLocation(this._splatProgram, "splatSize")) {
                var splatSizeX = this.gl.canvas.width  / this._splats_x;
                var splatSizeY = this.gl.canvas.height / this._splats_y;
                var splatSize = Math.max(splatSizeX, splatSizeY) * this._splatOverlap;
                if ( Math.abs(splatSizeX-splatSizeY) > 0.001 ) {
                    console.log("Viewport size and number of splats indicate that splats with non-unity aspect ratio has been requested!" +
                                " x) " + splatSizeX + " y) " + splatSizeY + " used) " +splatSize);
                }
                this.gl.uniform1f( this.gl.getUniformLocation(this._splatProgram, "splatSize"), splatSize );
            }
            if (this.gl.getUniformLocation(this._splatProgram, "splats_x")) {
                this.gl.uniform1f( this.gl.getUniformLocation(this._splatProgram, "splats_x"), this._splats_x );
            }
            if (this.gl.getUniformLocation(this._splatProgram, "splats_y")) {
                this.gl.uniform1f( this.gl.getUniformLocation(this._splatProgram, "splats_y"), this._splats_y );
            }
            if (this.gl.getUniformLocation(this._splatProgram, "splatOverlap")) {
                this.gl.uniform1f( this.gl.getUniformLocation(this._splatProgram, "splatOverlap"), this._splatOverlap );
            }

            this.gl.vertexAttribPointer( vertexPositionAttribute, 2, this.gl.FLOAT, false, 0, 0);
            this.gl.drawArrays(this.gl.POINTS, 0, this._splats_x*this._splats_y);
        }

        // console.log("rendering");
    }

});
