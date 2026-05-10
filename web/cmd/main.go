package main

import (
	"fmt"
	"html/template"
	"net/http"
	"os"

	log "github.com/gothew/l-og"
	"github.com/joho/godotenv"
)

var tmpl *template.Template

func init() {
	tmpl, _ = template.ParseGlob("public/*.html")
}

func loadEnv() {
	// Load environment variables or configuration here if needed
	err := godotenv.Load()
	if err != nil {
		log.Fatal("Error loading .env file")
	}
}

// TODO: add gracefull shutdown
func main() {
	loadEnv()
	mux := http.NewServeMux()

	mux.HandleFunc("/", HomePage)

	port := fmt.Sprintf(":%s", os.Getenv("PORT"))
	log.Infof("Server listen on port %s", port)
	if err := http.ListenAndServe(port, mux); err != nil {
		log.Errorf("server failed %+v", err)
	}
}

func HomePage(w http.ResponseWriter, r *http.Request) {
	tmpl.ExecuteTemplate(w, "index.html", nil)
}
