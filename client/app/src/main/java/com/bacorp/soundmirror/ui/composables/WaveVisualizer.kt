package com.bacorp.soundmirror.ui.composables

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import kotlin.math.PI
import kotlin.math.sin

@Composable
fun WaveVisualizer(
    modifier: Modifier = Modifier,
    amplitude: Float
) {
    val infiniteTransition = rememberInfiniteTransition()
    val phase by infiniteTransition.animateFloat(
        initialValue = 0f,
        targetValue = 1000f,
        animationSpec = infiniteRepeatable(
            animation = tween(
                durationMillis = (1600f / (2 * PI) * 1000f).toInt(),
                easing = LinearEasing
            ),
            repeatMode = RepeatMode.Restart
        ),
    )
    val color = MaterialTheme.colorScheme.primary

    Canvas(
        modifier = modifier
    ) {
        val width = size.width
        val height = size.height
        val centerY = height / 2
        val steps = (width / 32).toInt()

        drawWave(
            color = color.copy(alpha = 0.8f),
            amplitude = amplitude * 0.9f,
            frequency = 1.2f,
            phase = phase,
            centerY = centerY,
            width = width,
            steps = steps
        )

        drawWave(
            color = color.copy(alpha = 0.7f),
            amplitude = amplitude * 0.77f,
            frequency = 1.8f,
            phase = phase * 1.3f,
            centerY = centerY,
            width = width,
            steps = steps
        )

        drawWave(
            color = color.copy(alpha = 0.6f),
            amplitude = amplitude * 0.6f,
            frequency = 1.7f,
            phase = phase,
            centerY = centerY,
            width = width,
            steps = steps
        )

        drawWave(
            color = color.copy(alpha = 0.5f),
            amplitude = amplitude * 0.95f,
            frequency = 1.8f,
            phase = phase * 0.7f,
            centerY = centerY,
            width = width,
            steps = steps
        )
    }
}

fun DrawScope.drawWave(
    color: Color,
    amplitude: Float,
    frequency: Float,
    phase: Float,
    centerY: Float,
    width: Float,
    steps: Int
) {
    val path = Path().apply {
        val stepWidth = width / steps

        val x = stepWidth
        val y = centerY + amplitude *
                sin(2 * PI.toFloat() * x / width * frequency + phase)
        moveTo(x, y)

        for (i in 0..steps) {
            val x = i * stepWidth
            val y = centerY + amplitude *
                    sin(2 * PI.toFloat() * x / width * frequency + phase)
            lineTo(x, y)
        }
    }

    drawPath(
        path = path,
        color = color,
        style = Stroke(
            width = 4.dp.toPx(),
            cap = StrokeCap.Round
        )
    )
}
