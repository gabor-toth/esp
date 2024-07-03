import { Component } from '@angular/core';
import { RouterLink, RouterLinkActive, RouterModule, RouterOutlet } from '@angular/router';

@Component( {
  selector: 'app-root',
  standalone: true,
  imports: [ RouterModule, RouterLinkActive, RouterLink, RouterOutlet ],
  templateUrl: './app.component.html',
  styleUrl: './app.component.scss'
} )
export class AppComponent {
  title = 'Öntöző';
}
