// Shaders for the enemy-bot silhouette overlay (client-side, loopback + bots
// only - see CL_MaybeAddBotSilhouette in code/client/cl_cgame.c).
//
// The client clones each bot's already-posed player model and re-submits it
// with these as its customShader, tinted through shaderRGBA:
//   rgbGen entity   -> takes the colour from refEntity.shaderRGBA[0..2]
//   alphaGen entity -> takes the opacity from refEntity.shaderRGBA[3]
// Through-wall visibility comes from RF_DEPTHHACK set on the clone, not from
// anything in here.
//
// Ships as zz-bot-silhouette.pk3 so it also loads under sv_pure.

// Flat filled silhouette of the whole model - the see-through "blob" for a
// hidden bot.
botSilhouette
{
	// No cull line on purpose: the default is front-sided, which is what this
	// wants. With "cull none" every surface blends twice and the fill comes out
	// far denser than the chosen alpha. (There is no "cull front" token - the
	// parser only knows none/twosided/disable and back/backside/backsided.)
	{
		map $whiteimage
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
		alphaGen entity
	}
}

// Depth-only mask of the true (un-inflated) model. It writes depth but leaves
// the colour untouched (blendFunc GL_ZERO GL_ONE), so that where the contour
// hull overlaps the model body it loses the depth test and only the rim - the
// outline - survives. Paired with RF_DEPTHHACK on the clone, the mask sits in
// front of walls too, which is what lets the outline show through them.
//
// The sort must sit ABOVE every blended surface, not below it. ParseSort takes
// a bare number through atof, and the highest stock sort is SS_NEAREST (16), so
// 17 puts the mask after all world transparency. At a low sort its depth write
// would clip every later blended surface behind the bot - smoke, explosions,
// glass, grates - leaving a bot-shaped hole in them. The contour follows at 18,
// so the mask still carves the body before the rim is drawn.
botMask
{
	sort 17
	{
		map $whiteimage
		blendFunc GL_ZERO GL_ONE
		depthWrite
	}
}

// Inverted-hull contour: a frequency-0 deformVertexes wave pushes every vertex
// 2 units out along its normal, cull back keeps the reverse faces, so the
// inflated shell shows as a rim around the true model shape. Drawn after the
// mask (sort 18 against its 17) so the mask has already carved out the body.
botOutline
{
	sort 18
	deformVertexes wave 100 sin 2 0 0 0
	cull back
	{
		map $whiteimage
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
		alphaGen entity
	}
}
