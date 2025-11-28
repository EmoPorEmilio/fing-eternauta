# Download Dear ImGui files
$files = @(
    "imgui_draw.cpp",
    "imgui_widgets.cpp",
    "imgui_tables.cpp",
    "imgui_internal.h",
    "imstb_rectpack.h",
    "imstb_textedit.h",
    "imstb_truetype.h",
    "imgui_impl_opengl3.h",
    "imgui_impl_opengl3.cpp",
    "imgui_impl_sdl2.h",
    "imgui_impl_sdl2.cpp"
)

foreach ($file in $files) {
    $url = "https://raw.githubusercontent.com/ocornut/imgui/master/$file"
    Write-Host "Downloading $file..."
    Invoke-WebRequest -Uri $url -OutFile $file
}

Write-Host "All files downloaded!"
