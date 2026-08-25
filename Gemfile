# Local preview of the GitHub Pages site in static/. Not used by CI, which
# builds with the ghcr.io/actions/jekyll-build-pages container instead.
#
# Deliberately at the repo root rather than in static/: the CI action warns if
# it finds a Gemfile in the source directory that its own bundle cannot satisfy.
#
# The github-pages version is pinned to what that container ships, so a local
# build matches what gets published. Check it against the action's Gemfile at
# https://github.com/actions/jekyll-build-pages/blob/main/Gemfile when bumping
# the pin in .github/workflows/publish-pages.yml.
source "https://rubygems.org"

gem "github-pages", "232", group: :jekyll_plugins
# Removed from the standard library in Ruby 3.0, and `jekyll serve` needs it.
gem "webrick", "~> 1.8"
