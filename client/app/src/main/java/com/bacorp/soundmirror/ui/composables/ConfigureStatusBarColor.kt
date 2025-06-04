package com.bacorp.soundmirror.ui.composables

import android.graphics.Rect
import android.os.Build
import android.view.View
import android.view.Window
import android.view.WindowInsets
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.toArgb
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat

@Composable
fun ConfigureStatusBarColor(window: Window, isDark: Boolean) {
    val color = MaterialTheme.colorScheme.surface.toArgb()
    when {
        Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM -> {
            val decor = window.decorView

            val insetsController = WindowCompat.getInsetsController(window, decor)

            SideEffect {
                insetsController.isAppearanceLightStatusBars = !isDark
            }
            decor.setOnApplyWindowInsetsListener { view, insets ->
                val inset = insets.getInsets(WindowInsets.Type.statusBars()).top
                view.setPadding(view.paddingLeft, inset, view.paddingRight, view.paddingBottom)
                view.setBackgroundColor(color)
                view.setOnApplyWindowInsetsListener(null)
                insets
            }
            decor.requestApplyInsets()
        }

        else -> {
            val decorView = window.decorView
            val contentView = decorView.findViewById<View>(android.R.id.content)

            val originalPadding =
                Rect(
                    contentView.paddingLeft,
                    contentView.paddingTop,
                    contentView.paddingRight,
                    contentView.paddingBottom,
                )
            ViewCompat.setOnApplyWindowInsetsListener(contentView) { view, insets ->
                val statusBarInset = insets.getInsets(WindowInsetsCompat.Type.statusBars()).top
                view.setPadding(
                    originalPadding.left,
                    statusBarInset,
                    originalPadding.right,
                    originalPadding.bottom,
                )
                insets
            }

            WindowCompat.getInsetsController(window, window.decorView).apply {
                isAppearanceLightStatusBars = !isDark
            }
            ViewCompat.requestApplyInsets(decorView)

            @Suppress("DEPRECATION")
            window.statusBarColor = color
        }
    }
}
